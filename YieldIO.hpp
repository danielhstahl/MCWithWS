std::vector<SpotValue> populateYieldFromExternalSource(Date& dt, auto& data, auto& yldClass, auto& daysPlus){

    std::vector<SpotValue> SwapRates;
    std::vector<SpotValue> LiborRates;
    dt.setScale("day");
    std::vector<SpotValue> historical;
    rapidjson::Value::ConstMemberIterator itrs= data.FindMember("series");
    rapidjson::Value::ConstMemberIterator itry= data.FindMember("yield");

    if(itry!=data.MemberEnd() && itrs!=data.MemberEnd()){
      const rapidjson::Value& yield=itry->value;
      const rapidjson::Value& series=itrs->value["history"];
      int n=yield.Size();
      int m=series.Size();
      for(int i=0; i<n; ++i){
          if(strcmp(yield[i]["type"].GetString(), "Swap")==0){
              SwapRates.push_back(SpotValue(dt+yield[i]["days"].GetInt(), std::stod(yield[i]["value"].GetString())*.01));
          }
          else{
              LiborRates.push_back(SpotValue(dt+yield[i]["days"].GetInt(), std::stod(yield[i]["value"].GetString())*.01));
          }
      }
      //end deal with yield
      //Deal with historical

      std::string val;
      for(int i=0; i<m; ++i){
          val=series[i]["value"].GetString();
          if(val!="."&&val!=""){//nulls show up as "."
              historical.push_back(SpotValue(series[i]["date"].GetString(), std::stod(val)*.01));
          }
      }
      yldClass.computeSimpleSwapSpline(LiborRates, SwapRates, dt);
      daysPlus=data["series"]["days"].GetInt()/360.0;
      return historical;
    }
    else{
      return historical;
    }

}
