#define _USE_MATH_DEFINES
#define _WEBSOCKETPP_CPP11_THREAD_
#define _WEBSOCKETPP_CPP11_CHRONO_
#define _WEBSOCKETPP_CPP11_TYPE_TRAITS_
#define ASIO_STANDALONE
#define ASIO_HAS_STD_ARRAY
#define ASIO_HAS_STD_ATOMIC
#define ASIO_HAS_CSTDINT
#define ASIO_HAS_STD_ADDRESSOF
#define ASIO_HAS_STD_SHARED_PTR
#define ASIO_HAS_STD_TYPE_TRAITS
//#define RAPIDJSON_PARSE_ERROR_NORETURN(parseErrorCode,offset)
//#include <stdexcept>               // std::runtime_error

#include <set>
#include <asio.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/config/core.hpp>
#include <websocketpp/server.hpp>
#include <thread>
#include <iostream>
#include "BlackScholes.h"
#include <cfloat> //this is for DBL_MAX
#include "HullWhiteEngine.h"
#include <string>
#include "YieldIO.h"
#include "RealWorldMeasure.h"
#include "CurveFeatures.h"
#include "YieldSpline.h"
#include <map>
#include "HandlePath.h" //for creating sample paths
#include "MC.h" //monte carlo
#include "Histogram.h" //bins data
#include "SimulateNorm.h"
#include "document.h" //rapidjson
#include "writer.h" //rapidjson
#include "reader.h" //rapidjson
#include "stringbuffer.h" //rapidjson
#include "error/error.h" // rapidjson::ParseResult
#include "error/en.h" // rapidjson::ParseResult
struct ParseException : std::runtime_error, rapidjson::ParseResult {
  ParseException(rapidjson::ParseErrorCode code, const char* msg, size_t offset)
    : std::runtime_error(msg), ParseResult(code, offset) {}
};

typedef websocketpp::server<websocketpp::config::asio> server;
using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

struct pointToClass{ //this is data structure per connection
  YieldSpline yld;
  Date currDate;
  double daysDiff;
  std::vector<SpotValue> historical;
};

/*const std::string expectedInput="{\"getYield\":{\"series\":;
const rapidjson::Document jsonInput;
jsonInput.parse(expectedInput);*/
class WS{
private:
	//typedef std::set<connection_hdl,std::owner_less<connection_hdl>> con_list;
	server m_server;
  std::map<connection_hdl, pointToClass, std::owner_less<connection_hdl>> holdThreads;
	//con_list m_connections;
public:
	WS(){
		m_server.init_asio();
		m_server.set_open_handler(bind(&WS::on_open,this,::_1));
		m_server.set_close_handler(bind(&WS::on_close,this,::_1));
		m_server.set_message_handler(bind(&WS::on_message,this,::_1,::_2));
	}
  void getYield(connection_hdl connection, rapidjson::Value::ConstMemberIterator parms){
    auto ws=[&](const std::string& message){
      m_server.send(connection,message, websocketpp::frame::opcode::text);
    };
    //rapidjson::Document parms;
    //parms.Parse(msg->get_payload().c_str()); //large array
    Date currDate;
    double daysDiff;//years from now that libor rate goes till (typically 7 days divided by 360)
    auto& myDataStructure=holdThreads[connection]; //reference?
    myDataStructure.historical=populateYieldFromExternalSource(myDataStructure.currDate, parms->value, myDataStructure.yld, daysDiff);
    if(myDataStructure.historical.size()==0){
      ws("{\"Error\":\"Problem with yield\"}");
    }
    else{
      myDataStructure.daysDiff=daysDiff;
      myDataStructure.yld.getSpotCurve(ws);//send data to node
      myDataStructure.yld.getForwardCurve(ws); //send data ot node
    }
  }
  void getMC(connection_hdl connection, rapidjson::Value::ConstMemberIterator parms){
    auto ws=[&](const std::string& message){
      m_server.send(connection,message, websocketpp::frame::opcode::text);
    };
    //rapidjson::Document parms;
    //parms.Parse(msg->get_payload().c_str()); //large array
    
    HullWhiteEngine<double> HW;
    auto& currentThreadData=holdThreads[connection]; //should be a reference so no copying
    double r0=currentThreadData.yld.getShortRate(); //note we can change this here to an AutoDiff if we want sensitivities
    SimulateNorm rNorm;
    MC<double> monteC;
    std::vector<AssetFeatures> portfolio;
    currentThreadData.currDate.setScale("year");
    rapidjson::Value::ConstMemberIterator itrport= parms->value.FindMember("portfolio");
    rapidjson::Value::ConstMemberIterator itrglob= parms->value.FindMember("global");
    if(itrport!=parms->value.MemberEnd() && itrglob!=parms->value.MemberEnd()){
      auto& jsonPortfolio=itrport->value;
      auto& globalVars=itrglob->value;
      int n=jsonPortfolio.Size();
      for(int i=0; i<n; ++i){
        AssetFeatures asset;
        auto& indAsset=jsonPortfolio[i];
        auto iter=indAsset.FindMember("T");
        if(iter!=indAsset.MemberEnd()){
            asset.Maturity=currentThreadData.currDate+iter->value.GetDouble();
        }
        iter=indAsset.FindMember("k");
        if(iter!=indAsset.MemberEnd()){
            asset.Strike=iter->value.GetDouble();
        }
        iter=indAsset.FindMember("delta");
        if(iter!=indAsset.MemberEnd()){
            asset.Tenor=iter->value.GetDouble();
        }
        iter=indAsset.FindMember("Tm");
        if(iter!=indAsset.MemberEnd()){
            asset.UnderlyingMaturity=currentThreadData.currDate+iter->value.GetDouble();
        }
        asset.type=indAsset["type"].GetInt();
        portfolio.push_back(asset); //does this entail unessary copying?
      }
      double a=globalVars["a"].GetDouble(); //can be made autodiff too
      double sigma=globalVars["sigma"].GetDouble(); //can be made autodiff too
      double b=findHistoricalMean(currentThreadData.historical, currentThreadData.daysDiff, a);
      int m=0;
      if(globalVars.FindMember("n")!=parms->value.MemberEnd()){
          m=globalVars["n"].GetInt();
      }
      HW.setSigma(sigma);
      HW.setReversion(a);
      currentThreadData.currDate.setScale("day");
      Date PortfolioMaturity;
      auto iter=globalVars.FindMember("t");
      if(iter!=globalVars.MemberEnd()){
          PortfolioMaturity=currentThreadData.currDate+iter->value.GetInt();
      }
      monteC.setM(m);
      std::vector<Date> path=getUniquePath(portfolio, PortfolioMaturity);
      portfolio[0].currValue=HW.HullWhitePrice(portfolio[0], r0, currentThreadData.currDate, currentThreadData.currDate, currentThreadData.yld);
      double currentPortfolioValue=portfolio[0].currValue;
      for(int i=1; i<n;++i){
          portfolio[i].currValue+=HW.HullWhitePrice(portfolio[i], r0, currentThreadData.currDate, currentThreadData.currDate, currentThreadData.yld);
          currentPortfolioValue+=portfolio[i].currValue;
      }
      monteC.simulateDistribution([&](){
          return executePortfolio(portfolio, currentThreadData.currDate,
              [&](const auto& currVal, const auto& time){
                  double vl=rNorm.getNorm();
                  return generateVasicek(currVal, time, a, b, sigma, vl);
              },
              r0,
              path,
              [&](AssetFeatures& asset, auto& rate, Date& maturity,   Date& asOfDate){
                  return HW.HullWhitePrice(asset, rate, maturity, asOfDate, currentThreadData.yld);
              }
          );
      }, ws );
      std::vector<double> dist=monteC.getDistribution();
      double min=DBL_MAX; //purposely out of order because actual min and max are found within the function
      double max=DBL_MIN;
      double stdError=monteC.getError();
      double stdDev=stdError*sqrt(m);
      double exLoss=monteC.getEstimate();
      double VaR=monteC.getVaR(.99);
      std::stringstream wsMessage;
      VaR=VaR-currentPortfolioValue; //var is negative no?
      wsMessage<<"{\"Overview\":[{\"label\":\"Value At Risk\", \"value\":"<<VaR<<"}, {\"label\":\"Expected Return\", \"value\":"<<exLoss-currentPortfolioValue<<"}, {\"label\":\"Current Portfolio Value\", \"value\":"<<currentPortfolioValue<<"}]}";
      ws(wsMessage.str());
      double exLossCheck=0;//once this check is finished delete it
      double varianceCheck=0;//once this check is finished delete it
      double checkVaR=0;
      auto computeRiskContributions=[&](){
        std::stringstream wsMessage;
        wsMessage<<"{\"RC\":[";
        double c=(VaR-(exLoss-currentPortfolioValue))/(stdDev*stdDev);
        for(int i=0; i<(n-1); ++i){
          portfolio[i].covariance=(portfolio[i].covariance-portfolio[i].expectedReturn*exLoss)/((double)(m-1));
          portfolio[i].expectedReturn=portfolio[i].expectedReturn/m;
          wsMessage<<"{\"Asset\":"<<portfolio[i].type<<", \"Contribution\":"<<portfolio[i].expectedReturn-portfolio[i].currValue+c*portfolio[i].covariance<<"}, ";
          checkVaR+=portfolio[i].expectedReturn-portfolio[i].currValue+c*portfolio[i].covariance;
          varianceCheck+=portfolio[i].covariance;
        }
        portfolio[n-1].covariance=(portfolio[n-1].covariance-portfolio[n-1].expectedReturn*exLoss)/((double)(m-1));
        portfolio[n-1].expectedReturn=portfolio[n-1].expectedReturn/m;
        wsMessage<<"{\"Asset\":"<<portfolio[n-1].type<<", \"Contribution\":"<<portfolio[n-1].expectedReturn-portfolio[n-1].currValue+c*portfolio[n-1].covariance<<"}]}";
        checkVaR+=portfolio[n-1].expectedReturn-portfolio[n-1].currValue+c*portfolio[n-1].covariance;
        varianceCheck+=portfolio[n-1].covariance;
        std::cout<<"exloss: "<<exLoss<<std::endl;
        std::cout<<"VaR: "<<VaR<<" varCheck: "<<checkVaR<<std::endl;
        std::cout<<"variance: "<<stdDev*stdDev<<" varianceCheck: "<<varianceCheck<<std::endl;
        ws(wsMessage.str());
      };
      computeRiskContributions();
      binAndSend(ws, min, max, dist); //send histogram to node
    }
    else{
      ws("{\"Error\":\"Problem with MC\"}");
    }
  }
  void on_open(connection_hdl hdl) {
    pointToClass myDataStructure;
    holdThreads[hdl]=myDataStructure;
    m_server.send(hdl,"[\"yield\", \"mc\"]", websocketpp::frame::opcode::text);/*This is a very basic attempt!  in real applications this would say all the types expeted as inputs.  In real applications this is likely to be not very many inputs since all the "large" inputs will be from a database (eg positions/balances)*/
	}
	void on_close(connection_hdl hdl) {
    holdThreads.erase(hdl);
	}
	void on_message(connection_hdl hdl, server::message_ptr msg) {
    rapidjson::Document parms;
    auto ws=[&](const std::string& message){
      m_server.send(hdl, message, websocketpp::frame::opcode::text);
    };
    try{
      if(parms.Parse(msg->get_payload().c_str()).HasParseError()){ //parms should de-allocate after detaching threads since its not a pointer
        throw std::invalid_argument("JSON not formatted correctly");
      }
      rapidjson::Value::ConstMemberIterator itr1 =parms.FindMember("yield");
      rapidjson::Value::ConstMemberIterator itr2 =parms.FindMember("mc");
      if(itr1!=parms.MemberEnd()){
        std::thread myThread(&WS::getYield, this, hdl, itr1); 
        myThread.detach();
      }
      else if(itr2!=parms.MemberEnd()){
        std::thread myThread(&WS::getMC, this, hdl, itr2);
        myThread.detach();
      }
      else{
        ws("{\"Error\":\"Data is in wrong format!\"}");
      }
    }
		catch(const std::exception& e) { 
      std::stringstream wsMessage;
      wsMessage<<"{\"Error\":\""<<e.what()<<"\"}";
      ws(wsMessage.str());
    }
	}
	void run(uint16_t port) {
			m_server.listen(port);
			m_server.start_accept();
			m_server.run();
	}
};
int main(int argc, char* argv[]){
  WS server;//([&](auto& server, auto& connection));
  server.run(atoi(argv[1]));//give port to the program
}
