import sympy
import math
from math import log, sqrt, ceil

class OQSORT:
    """
    implementation of OQSORT
    """
    def __init__(self, N, M, B):
        self.N = N
        self.M = M
        self.B = B
        # OQSORT params
        self.ALPHA, self.BETA, self.P, self.IdealCost = -1, -1, -1, -1
        # used for 2 level, 2nd sample params
        self._alpha, self._beta, self._p, self._cost, self._is_tight = -1, -1, -1, -1, -1

    def onelevel(self, N, is_tight, kappa=27.8):
        N, M, B = self.N, self.M, self.B 
        x = sympy.Symbol('x')
        y = x**2*(1-x)-2*(1+x)**2*(1+x/2)*(1+2*x)**2*N*B/M/M*(kappa+1+2*log(N/M))
        res = sympy.solveset(y, x, sympy.Interval(0,1))
        if len(res) == 0:
            raise ValueError("N too large!")
        beta = min(res)
        alpha = (kappa+1+log(N))*4*(1+beta)*(1+2*beta)/beta/beta/M
        p = ceil((1+2*beta)*N/M)
        if is_tight:
            cost = 5+4*beta
        else:
            cost = 4+6*beta+alpha*B
        print("One: alpha=%f, beta=%f, p=%d, cost=%f" % (alpha, beta, p, cost))
        self.ALPHA, self.BETA, self.P, self.IdealCost = alpha, beta, p, cost
        self.sortId = not is_tight
        self.is_tight = is_tight
        return self.ALPHA, self.BETA, self.P, self.IdealCost

    def twolevel(self, is_tight, kappa=27.8):
        N, M, B = self.N, self.M, self.B
        print("Calculating Parameters")
        x = sympy.Symbol('x')
        g = x**2*(1-x)/(1+x)**2/(1+x/2)/(1+2*x)
        y = g**2-4*B*B*(1+2*x)*N/M**3*(kappa+2+1.5*log(N/M))**2
        res = sympy.solveset(y, x, sympy.Interval(0,1))
        if len(res) == 0:
            raise ValueError("N too large!")
        beta = min(res)
        alpha = (kappa+2+log(N))*4*(1+beta)*(1+2*beta)/beta/beta/M 
        p = ceil(sqrt((1+2*beta)*N/M))
        self._alpha, self._beta, self._p, self._cost = self.onelevel(alpha*N, False, kappa + 1)
        # print("alpha1=%f, beta1=%f, p1=%d, cost1=%f" % (_alpha, _beta, _p, _cost))
        if is_tight:
            cost = 7+7*alpha+8*beta
        else:
            cost = 6+(6+B)*alpha+10*beta
        self.ALPHA, self.BETA, self.P, self.IdealCost = alpha, beta, p, cost
        print("Two: alpha=%f, beta=%f, p=%d, cost=%f" % (alpha, beta, p, cost))
        self.sortId = not is_tight
        self.is_tight = is_tight
        
if __name__ == '__main__':
    M = 1048576 # ODS: 8388608, BS: 4194304
    N, B, is_tight = 8*M, 2, 0
    print("N, M, B: " + str(N) + ', ' +str(M) + ', ' + str(B)) 
    # N, M, B, is_tight = 335544320, 16777216, 4, 1
    sortCase1 = OQSORT(N, M, B)
    if N / M <= 128:
        sortCase1.onelevel(N, is_tight)
        f = open("/home/chenbingnan/mysamples/OQSORT/params.txt", "w")
        f.write(str(N) + '\n')
        f.write(str(M) + '\n')
        f.write(str(B) + '\n')
        f.write(str(sortCase1.ALPHA) + '\n')
        f.write(str(sortCase1.BETA) + '\n')
        f.write(str(sortCase1.P))
    else:
        sortCase1.twolevel(is_tight)
        f = open("/home/chenbingnan/mysamples/OQSORT/params.txt", "w")
        f = open("/home/chenbingnan/mysamples/OQSORT/params.txt", "w")
        f.write(str(N) + '\n')
        f.write(str(M) + '\n')
        f.write(str(B) + '\n')
        f.write(str(sortCase1.ALPHA) + '\n')
        f.write(str(sortCase1.BETA) + '\n')
        f.write(str(sortCase1.P) + '\n') 
        f.write(str(sortCase1._alpha) + '\n')
        f.write(str(sortCase1._beta) + '\n')
        f.write(str(sortCase1._p))