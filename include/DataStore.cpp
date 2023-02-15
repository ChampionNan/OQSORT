#include "DataStore.h"
#include "common.h"

using namespace std;


DataStore::DataStore(EncOneBlock **arrayAddr, int64_t N, int64_t M, int B) : arrayAddr{arrayAddr}, N{N}, M{M}, B{B} {}

DataStore::~DataStore() {
  for (int id : delArray) {
    delete [](arrayAddr[id]);
  }
  cout << "Delete DataStore objects\n";
}

void DataStore::init(int structureId, int64_t size) {
  int64_t i, j, blockNumber;
  // 1. allocate memory
  arrayAddr[structureId] = new EncOneBlock[size];
  EncOneBlock *addr = arrayAddr[structureId];
  delArray.push_back(structureId);
  // 2. value initialization
  // #pragma omp parallel for
  std::random_device rd;
  std::mt19937 rng{rd()};
  std::uniform_int_distribution<int> dist{std::numeric_limits<int>::min(), std::numeric_limits<int>::max()-1};
  for (int64_t i = 0; i < size; ++i) {
    addr[i].sortKey = dist(rng);
    addr[i].primaryKey = i;
    // addr[i].payLoad = DUMMY<int>();
    memset(addr[i].payLoad, 0, PAYLOAD * sizeof(int));
    addr[i].randomKey = 0; // also used for dummy flag
  }
}

void DataStore::print(int structureId, int64_t size, OutputType outputtype, const char *filepath) {
  EncOneBlock *addr = arrayAddr[structureId];
  if (outputtype == TERMINAL) {
    for (int i = 0; i < size; ++i) {
      cout << "addr[" << i << "]: (" << addr[i].sortKey;
      cout << ", " << addr[i].primaryKey << ")" << endl;
    }
    cout << endl;
  } else if (outputtype == FILEOUT) {
    ofstream fout(filepath);
    int64_t outsize = size / 1000;
    for (int64_t i = 0; i < outsize; ++i) {
      fout << "addr[" << i << "]: (" << addr[i].sortKey;
      fout << ", " << addr[i].primaryKey << ")" << endl;
    }
    fout << endl;
    fout.close();
  }
}

void DataStore::test(int structureId, int64_t size, SortType sorttype) {
  EncOneBlock *addr = arrayAddr[structureId];
  int v = 1;
  for (int i = 0; i < size; ++i) {
    if (addr[i].sortKey == DUMMY<int>() && sorttype != ODSLOOSE) {
      throw "Dummy found in non-loose sort! ";
    } else if (addr[i].sortKey != DUMMY<int>() && addr[i].sortKey != v++) {
      throw "Value error";
    }
  }
}

int64_t DataStore::RandRange(int64_t start, int64_t end) {
  std::uniform_int_distribution<int64_t> distr(start, end - 1);
  return distr(rng);
}
