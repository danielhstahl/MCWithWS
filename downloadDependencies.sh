git clone https://github.com/zaphoyd/websocketpp.git ../websocketpp #header only
git clone https://github.com/miloyip/rapidjson.git ../rapidjson #header only
git clone https://github.com/chriskohlhoff/asio.git ../asio #header only..but include needs to go deep
hg clone https://bitbucket.org/eigen/eigen/ ../eigen #header only
git clone https://github.com/phillyfan1138/AutoDiff ../AutoDiff
g++ -O3 -std=c++14 -o ../AutoDiff/AutoDiff.o -c ../AutoDiff/AutoDiff.cpp
ar rcs ../AutoDiff/libAutoDiff.a ../AutoDiff/AutoDiff.o
git clone https://github.com/phillyfan1138/BinomialTree ../BinomialTree #header only
git clone https://github.com/phillyfan1138/FixedIncomeUtilities ../FixedIncomeUtilities #header only
git clone https://github.com/phillyfan1138/GaussNewton ../GaussNewton
g++ -O3 -std=c++14 -o ../GaussNewton/Newton.o -c ../GaussNewton/Newton.cpp -I../eigen -I../AutoDiff -L../AutoDiff -lAutoDiff -fopenmp
ar rcs ../GaussNewton/libNewton.a ../GaussNewton/Newton.o
git clone https://github.com/phillyfan1138/MiscellaniousUtilities ../MiscellaniousUtilities
g++ -O3 -std=c++14 -fPIC -o ../MiscellaniousUtilities/Date.o -c ../MiscellaniousUtilities/Date.cpp -fopenmp
ar rcs ../MiscellaniousUtilities/libDate.a ../MiscellaniousUtilities/Date.o
git clone https://github.com/phillyfan1138/MonteCarlo ../MonteCarlo 