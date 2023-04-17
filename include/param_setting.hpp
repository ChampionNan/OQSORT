// reference from xxx

#include "common.h"

static double addLogs(double logA, double logB) {
  double bigger = std::max(logA, logB);
  double smaller = std::min(logA, logB);
  if (bigger == -INFINITY) {
    return bigger;
  }
  if (bigger - smaller > 100) {
    return bigger;
  }
  return bigger + log2(1.0 + pow(2, (smaller - bigger)));
}

static double logCombin(uint64_t k, uint64_t n) {
  assert(k <= n);
  if (k * 2 > n) {
    k = n - k;
  }
  double res = (lgamma(n + 1) - lgamma(k + 1) - lgamma(n - k + 1)) / log(2);
  return res;
}

static double binomLogPmf(uint64_t k, uint64_t n, double p) {
  double logCkn = logCombin(k, n);
  if (p > 1e-5) {
    return logCkn + k * log2(p) + (n - k) * log2(1-p);
  }
  return logCkn + k * log2(p) - (n-k) / log(2) * (p + p*p/2 + p*p*p/3);
}

static double binomLogSf(uint64_t k, uint64_t n, double p) {
  double sf = -INFINITY;
  double pmf = binomLogPmf(k, n, p);
  double eps = pmf - 40;
  while (pmf > eps && k < n) {
    ++k;
    pmf += log2((double)(n-k+1)/k) + log2(p/(1-p));
    sf = addLogs(sf, pmf);
  }
  return sf;
}

static double binomLogCdf(uint64_t k, uint64_t n, double p) {
  double sf = -INFINITY;
  double pmf = binomLogPmf(k, n, p);
  double eps = pmf - 40;
  while (pmf > eps) {
    sf = addLogs(sf, pmf);
    if (k == 0) break;
    pmf -= log2((double)(n-k+1)/k) + log2(p/(1-p));
    --k;
  }
  return sf;
}

static double hypergeomLogPmf(uint64_t k, uint64_t M, uint64_t n, uint64_t N) {
  assert(N <= M);
  if (k > N || k > n) {
    return -INFINITY;
  }
  double logCkn = logCombin(k, n);
  double logCN_kM_n = logCombin(N - k, M - n);
  static size_t cachedM = 0;
  static size_t cachedN = 0;
  static size_t cachedLogMN = 0;
  double logCMN;
  if (M == cachedM && N == cachedN) {
    logCMN = cachedLogMN;
  } else {
    logCMN = logCombin(N, M);
    cachedM = M;
    cachedN = N;
    cachedLogMN = logCMN;
  }
  return logCkn + logCN_kM_n - logCMN;
}

static double hypergeomLogSf(uint64_t k, uint64_t M, uint64_t n, uint64_t N) {
  double sf = -INFINITY;
  double pmf = hypergeomLogPmf(k, M, n, N);
  double eps = pmf - 40;
  while (pmf > eps && k < n) {
    ++k;
    pmf += log2((double)(n-k+1)/k) - log2((double)(M-n-N+k)/(N-k+1));
    sf = addLogs(sf, pmf);
  }
  return sf;
}

template<typename T, class Check>
static T lowerBound(T left, T right, const Check& satisfy, T prec = 1) {
  while (true) {
    T mid = (left + right) / 2;
    if (satisfy(mid)) {
      right = mid;
      if (right - left <= prec) {
        return mid;
      }
    } else {
      left = mid;
      if (right - left <= prec) {
        return right;
      }
    }
  }
}

static double logP_Overflow_when_t1_data_between_pivots(size_t total, double p, double eps, size_t t1, size_t B) {
  double prob = hypergeomLogSf(int(B*(1+eps)), total, t1, B*p) + log2(total / B);
  return std::min(0.0, prob);
}

static double logP_more_than_t1_data_between_pivots(size_t total, double sampleratio, double p, size_t t1) {
  double prob = binomLogCdf(ceil(total*sampleratio/p), t1, sampleratio) + log2(total);
  return std::min(0.0, prob);
}

static double logP_t1_data_between_pivots(size_t total, double sampleratio, double p, size_t t1) {
  double prob = binomLogPmf(ceil(total*sampleratio/p)-1, t1-1, sampleratio) + log2(total) + 2 * log2(sampleratio);
  return std::min(0.0, prob);
}

static double failureProbOQPartition(size_t N, size_t M, size_t layer, double alpha, double eps, double target = -60) {
  size_t logTotal = ceil(log2(N));
  size_t numTotalBatch = ceil_divide(N * (1 + eps), M);
  M = ceil_divide(N * (1 + eps), numTotalBatch);
  double logM = log2(M);
  size_t way = ceil(pow(numTotalBatch, 1.0 / layer));
  double logWay = log2(way);
  size_t p = numTotalBatch;
  double q = -INFINITY;
  while (p > 1) {
    size_t B = ceil_divide(N, numTotalBatch * std::min(way, p));

    auto satisfy_start = [&](int64_t negt1) {
      return logP_Overflow_when_t1_data_between_pivots(N, p, eps, -negt1, B) < target - 4;
    };
    size_t t1_start = -lowerBound(-(int64_t)(N*(1+eps)/p), -(int64_t)(N/p), satisfy_start);
    auto satisfy_end = [&](int64_t t1) {
      return logP_more_than_t1_data_between_pivots(N, alpha, p, t1) < target - 4;
    };
    size_t t1_end = lowerBound((int64_t)(N/p), (int64_t)(N*(1+eps)/p), satisfy_end);
    if (t1_start >= t1_end) {
      t1_start = t1_end = (t1_start + t1_end) / 2;
    }
    double qLayer = logP_Overflow_when_t1_data_between_pivots(N, p, eps, t1_start-1, B);
    qLayer = addLogs(qLayer, logP_more_than_t1_data_between_pivots(N, alpha, p, t1_end));

    size_t step = std::max(1UL, (t1_end - t1_start) / 100);
    for (size_t t1 = t1_start; t1 < t1_end; t1 += step) {
      double qi = logP_t1_data_between_pivots(N, alpha, p, t1);
      double qj = logP_Overflow_when_t1_data_between_pivots(N, p, eps, t1 + step, B);
      qLayer = addLogs(qLayer, qi + qj + log2(std::min(step, t1_end - t1)));
    }
    // TODO: remove this
    qLayer += log2(log2(std::min(way, p)));
    q = addLogs(q, qLayer);
    p /= way;
  }
  return q;
}

static double IOCostOQSort(size_t N, size_t M, size_t layer, double recursive_ratio, double eps) {
  return N * (1 + recursive_ratio) * (2 * layer * (1 + eps) + 3 + recursive_ratio);
}

static double ComputeCostOQSort(size_t N, size_t M, size_t layer, double recursive_ratio, double eps) {
  size_t numTotalBatch = ceil_divide(N * (1 + eps), M);
  M = ceil_divide(N * (1 + eps), numTotalBatch);
  double logM = log2(M);
  size_t way = ceil(pow(numTotalBatch, 1.0 / layer));
  double logWay = log2(way);
  size_t cost = (1 + recursive_ratio) * numTotalBatch * (layer*0.5*M*(logM-logWay/2)*logWay + 0.25*M*logM*(logM+1));
  return cost;
}

static double TotalCostOQSort(size_t N, size_t M, size_t layer, double recursive_ratio, double eps) {
  double ioCost = IOCostOQSort(N, M, layer, recursive_ratio, eps);
  double computeCost = ComputeCostOQSort(N, M, layer, recursive_ratio, eps);
  return ioCost * 28 + computeCost;
}

static OQSortParams bestOQSortParams(int64_t &N, int64_t &M, double target = -60) {
  static const double eps_prec = 1e-4;
  static const double slack_sampling_prec = 1e-4;
  static const double alpha_prec = 1e-4;
  static const double logM_prec = 0.25;
  const size_t layer_max = std::max(1UL, (size_t)pow(log(N)/log(M), 2));
  OQSortParams bestParams = {};
  assert(N >= M);
  double bestCost = (double)INFINITY;
  for (size_t layer = 1; layer <= layer_max; ++layer) {
    OQSortParams params = {};
    auto MTarget = [&](int64_t M) {
      auto alphaTarget = [&](double alpha) {
        auto samplingSatisify = [&](double slack) {
          return binomLogSf(slack * M * alpha, M, alpha) + log2(ceil_divide(N, M)) < target - 4;
        };
        double slack_sampling = lowerBound(1.0, 1.5, samplingSatisify);
        auto epsSatisfy = [&](double _eps) {
          return failureProbOQPartition(N, M, layer, alpha, _eps) < target - 0.05;
        };
        double eps = lowerBound(0.0, 1.0, epsSatisfy, eps_prec);
        if (eps >= 1.0 - 2*eps_prec) {
          eps = lowerBound(0.0, 10.0, epsSatisfy, eps_prec*10);
        }
        params.gamma = eps;
        params.beta = slack_sampling;
        double cost = TotalCostOQSort(N, M, layer, alpha * slack_sampling, eps);
        return cost;
      };
      static const double alpha_min = 0.01;
      static const double alpha_max = 0.1;
      double best_alpha = alpha_min;
      double min_cost = INFINITY;
      for (double alpha = alpha_min; alpha <= alpha_max; alpha += 0.01) {
        double cost = alphaTarget(alpha);
        if (cost < min_cost) {
          best_alpha = alpha;
          min_cost = cost;
        } else {
          break;
        }
      }
      params.alpha = best_alpha;
      return alphaTarget(best_alpha);
    };
    size_t bestM = M;
    double cost = MTarget(bestM);
    params.M = bestM;
    if (cost < bestCost) {
      bestCost = cost;
      params.layer = layer;
      bestParams = params;
    }
  }
  bestParams.P = ceil(pow((1+bestParams.gamma)*N/M, 1.0/(bestParams.layer-0.2)));
  return bestParams;
}


// TODO: ADjust parameters, alpha, beta, gamma, M and P
// TODO: Check if current layer is 1