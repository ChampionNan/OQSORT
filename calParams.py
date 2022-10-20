import sympy
import math

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
        x = sympy.Symbol('x')
        g = x ** 2 * (1 - x) / (1 + x) ** 2 / (1 + x / 2) / (1 + 2 * x)
        y = g - 2 * (1 + 2 * x) * N * self.B / self.M / self.M * (kappa + 1 + 2 * math.log(N / self.M))
        res = sympy.solveset(y, x, sympy.Interval(0, 1))
        if len(res) == 0:
            raise ValueError("N too large!")
        beta = min(res)
        alpha = (kappa + 1 + math.log(N)) * 4 * (1 + beta) * (1 + 2 * beta) / beta / beta / self.M
        if alpha * N > self.M - self.B:
            raise ValueError("N too large!")
        p = math.ceil((1 + 2 * beta) * N / self.M)
        if is_tight:
            cost = 7 + 4 * beta
        else:
            cost = 6 + 6 * beta + alpha * self.B
        print("One: alpha=%f, beta=%f, p=%d, cost=%f" % (alpha, beta, p, cost))
        self.ALPHA, self.BETA, self.P, self.IdealCost = alpha, beta, p, cost
        self.sortId = not is_tight
        self.is_tight = is_tight
        return self.ALPHA, self.BETA, self.P, self.IdealCost

    def twolevel(self, is_tight, kappa=27.8):
        print("Calculating Parameters")
        x = sympy.Symbol('x')
        g = x ** 2 * (1 - x) / (1 + x) ** 2 / (1 + x / 2) / (1 + 2 * x)
        y = g ** 2 - 4 * self.B * self.B * (1 + 2 * x) * self.N / self.M ** 3 * (
                kappa + 2 + 1.5 * math.log(self.N / self.M)) ** 2
        res = sympy.solveset(y, x, sympy.Interval(0, 1))
        if len(res) == 0:
            raise ValueError("N too large!")
        beta = min(res)
        alpha = (kappa + 2 + math.log(self.N)) * 4 * (1 + beta) * (1 + 2 * beta) / beta / beta / self.M
        p = math.ceil(math.sqrt((1 + 2 * beta) * self.N / self.M))
        self._alpha, self._beta, self._p, self._cost = self.onelevel(alpha*self.N, False, kappa + 1)
        # print("alpha1=%f, beta1=%f, p1=%d, cost1=%f" % (_alpha, _beta, _p, _cost))
        if is_tight:
            cost = 9 + 7 * alpha + 8 * beta
        else:
            cost = 8 + (6 + self.B) * alpha + 10 * beta
        self.ALPHA, self.BETA, self.P, self.IdealCost = alpha, beta, p, cost
        print("Two: alpha=%f, beta=%f, p=%d, cost=%f" % (alpha, beta, p, cost))
        self.sortId = not is_tight
        self.is_tight = is_tight
        
if __name__ == '__main__':
    M = 16777216
    N, B, is_tight = 8*M, 4, 1
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