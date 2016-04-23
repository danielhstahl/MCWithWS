LDFLAGS=-L../GaussNewton -lNewton -L../MiscellaniousUtilities -lDate  -L../eigen -L../AutoDiff -lAutoDiff
INCLUDES=-I../GaussNewton -I../MiscellaniousUtilities -I../FixedIncomeUtilities -I../BinomialTree -I../rapidjson/include/rapidjson -I../AutoDiff -I../MonteCarlo -I../websocketpp -I../asio/asio/include -I../eigen

MarketRisk: main.o
	g++ -std=c++14 -O3  -w -Wall -pthread -fPIC main.o  $(LDFLAGS) $(INCLUDES) -o MarketRisk -fopenmp

main.o: main.cpp BlackScholes.h BlackScholes.hpp HullWhite.h HullWhite.hpp HullWhiteEngine.h HullWhiteEngine.hpp YieldIO.h YieldIO.hpp RealWorldMeasure.h RealWorldMeasure.hpp
	g++ -std=c++14 -O3  -w -Wall -c -pthread -fPIC main.cpp $(LDFLAGS) $(INCLUDES) -fopenmp

clean:
	-rm *.o MarketRisk
