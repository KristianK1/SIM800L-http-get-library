//#include "BluetoothSerial.h"

//BluetoothSerial SerialBT;

class AT_command{
private:
  String send;
  String expected;
  int seconds; //how many seconds should it take
  String error_mes; 
  int steps_back;
public:
  AT_command(String mSend, String mExpected, int mSeconds, String mError, int mSteps): send(mSend), expected(mExpected), seconds(mSeconds), error_mes(mError), steps_back(mSteps){}
  void setSend(String mSend){
    send=mSend;
  }
  void setExpected(String mExpected){
    expected=mExpected;
  }
  void setSeconds(int mSeconds){
    seconds=mSeconds;
  }
  String getSend(){ return send;}
  String getExpected(){ return expected;}
  int getSeconds(){ return seconds;}
  String getErrorMes(){ return error_mes;}
  int getStepsBack(){ return steps_back;}
};




class Progress{
private:
  int part; 
  int stage;
  int repeat;
  long int timeStamp;
  
  // part=0 - waiting for reset completion
  // part=1 - setup commands
  // part=2 - connect commands
  // part=3 - access commands
public:
  const int setup_size=5;              //BITNO ZA PROMJENITI NAKON MIJENJANJA AT NAREDBI
  const int connect_size=2;
  const int access_size=7;  
  
  AT_command AT_commands_setup[5]{
      AT_command("AT",                                     "OK",        5, "ERROR", 0),
      //AT_command("AT+CSQ",                                 "+CSQ:",     5, "ERROR", 1),
      //AT_command("AT+CGATT?",                              "+CGATT:",   2, "ERROR", 1),
      AT_command("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"",      "OK",       45, "ERROR", 1),
      AT_command("AT+SAPBR=3,1,\"APN\",\"TM\"",            "OK",        2, "ERROR", 1),
      AT_command("AT+SAPBR=3,1,\"USER\",\"\"",             "OK",        2, "ERROR", 1),
      AT_command("AT+SAPBR=3,1,\"PWD\",\"\"",              "OK",        2, "ERROR", 1)
    };
  
  AT_command AT_commands_connect[2]{
    AT_command("AT+SAPBR=1,1",                             "ERROR",     20,"OK",    0),
    AT_command("AT+SAPBR=2,1",                             "1,1"       ,3, "1,3",   1)
  };
  
  AT_command AT_commands_access[7]{
      AT_command("AT+HTTPTERM",                            "ERROR",     2, "",      0),
      AT_command("AT+HTTPINIT",                            "OK",        1, "ERROR", 1),
      AT_command("AT+HTTPPARA=\"CID\",1",                  "OK",        2, "ERROR", 1),
      AT_command("LINK",                                   "OK",        2, "ERROR", 2),
      AT_command("AT+HTTPACTION=1",                        "20",        2, "6",     3),
      AT_command("AT+HTTPREAD=0,20",                       "HTTPREAD",  2, "",      4),
      AT_command("AT+HTTPTERM",                            "OK",        2, "",     -1)
    }; 
    
  Progress(): part(0), stage(0), repeat(0){
  }
  void setPart(int mPart){ part=mPart;}
  void setStage(int mStage){ stage=mStage;}
  void setRepeat(int mRepeat){ repeat=mRepeat;}
  void setTimeStamp(int mTimeStamp){ timeStamp=mTimeStamp;}
  
  int getPart(){ return part;}
  int getStage(){ return stage;}
  int getRepeat(){ return repeat;}
  int getTimeStamp(){ return timeStamp;}
  
  void incrementStage(){ stage++;}
  void incrementPart(){ part++;}
  void incrementRepeat(){ repeat++;}
  void decrementStage(){ stage--;}

};


  
class SIM800L_S2{
private:
  bool GSM_on;
protected:
  Progress progress;
  String *recived;
  String link;
  int power_pin;
  long int timer_ON;
  long int access_timer;

  int AcontainsB(String A, String B){
    
    if(A.length()<B.length()) {
      return 0;
    }
    for(unsigned int i=0;i<=(A.length()-B.length());i++){
      String C="";
      for(unsigned int j=0;j<B.length();j++){
        C+=A[i+j];
      }
      if(C==B) return 1;
    }
    return 0;
  }
  void Send(String out){
    Serial2.println(out);
    //send_error_message(out);
    progress.setTimeStamp(millis());
  }
  void deleteRecive(){ *recived="";} 
  void Recive(String *recive){
    while(Serial2.available()){
      int c=Serial2.read();
      if(c!=10&&c!=13) {
        *recive=*recive+(char)c;
        //send_error_message(*recive);
      }
    }
    //SerialBT.print(*recive);
  }    
  int GSM_Reset_happend(String recived) {
    if(AcontainsB(recived,"CPIN")) {
      return 1;
    }
    if(AcontainsB(recived,"Ready")){
      return 1;
    }
    if(AcontainsB(recived,"CALL")){
      return 1;
    }
    if(AcontainsB(recived,"SMS")){
      return 1;
    }
    return 0;
  }
  
public:
  SIM800L_S2(int pin){
    power_pin=pin;
    pinMode(power_pin, OUTPUT); //32
    turn_off();
    Serial2.begin(9600);
    progress.setPart(1);
    recived=new String();
  }

  virtual ~SIM800L_S2(){
    delete(recived);
  }

  void reset_recive(){
    Recive(recived);
    deleteRecive();
  }
  /*void setPowerpin(int pin){ 
    power_pin=pin;
    pinMode(32, OUTPUT);
    turn_off();
  }*/
  
  void GSM_power(bool state){
    if(state==true){
      turn_on();
    }
    else{
      turn_off();
    }
  }

  void turn_on(){
    digitalWrite(power_pin, LOW);
    GSM_on=true;
    //SerialBT.println("upaljen");
    progress.setPart(1);
    progress.setStage(0);
  }
  void turn_off(){
    GSM_on=false;
    digitalWrite(power_pin, HIGH);
  }
  
  void setLink(String new_link){
    link=new_link;
  }
  String getLink(){
    return link;
  }

  void GSM_autoshutdown(){
    if(getLink()==""){
      if(millis()-timer_ON>4*60*1000){ //4 minute
        if(GSM_on==true){
          GSM_autoshutdown_main();
        }
      }
    }
    
  }

  long int getAccessTimer(){
    return access_timer;
  }
  void setAccessTimer(){
    access_timer=millis();
  }
  virtual void GSM_autoshutdown_main()=0;
  virtual void send_error_message(String message) = 0;
  
  int access(){
    deleteRecive();
    Recive(recived);
    if(GSM_Reset_happend(*recived)){
      progress.setPart(1);
      progress.setStage(0);
      progress.setRepeat(0);
      delay(200);
      Recive(recived);
      deleteRecive();
      Send(progress.AT_commands_setup[0].getSend());
      //SerialBT.println("reset happend");
      return 1;
    }

    if(progress.getPart()==0){
      delay(200);
      Recive(recived);
      deleteRecive();
      Send("AT");
      return 0;
    }
    if(progress.getPart()==1){
        if(AcontainsB(*recived, progress.AT_commands_setup[progress.getStage()].getExpected())){
          progress.incrementStage();
          if(progress.getStage()>=progress.setup_size){
            progress.setPart(2);
            progress.setStage(0);
            progress.setRepeat(0);
            deleteRecive();
            Send(progress.AT_commands_connect[0].getSend());
            return 1;
          }
          deleteRecive();
          Send(progress.AT_commands_setup[progress.getStage()].getSend());
        }
        else if(AcontainsB(*recived, progress.AT_commands_setup[progress.getStage()].getErrorMes())){
          progress.setRepeat(progress.getRepeat()+1);
          //SerialBT.println("error code");
          if(progress.getRepeat()>=10){
            for(int i=0;i<progress.AT_commands_setup[progress.getStage()].getStepsBack();i++){
              progress.decrementStage();
              deleteRecive();
              Send(progress.AT_commands_setup[0].getSend());
              return 2;
            }  
          }
          deleteRecive();
          Send(progress.AT_commands_setup[progress.getStage()].getSend());
          return 1;
        }
        else if(millis()-progress.getTimeStamp()>progress.AT_commands_setup[progress.getStage()].getSeconds()*1000){
          progress.setRepeat(progress.getRepeat()+1);
          //SerialBT.println("timeout");
          if(progress.getRepeat()>=10){
            for(int i=0;i<progress.AT_commands_setup[progress.getStage()].getStepsBack();i++){
              progress.decrementStage();
            }
            deleteRecive();
            Send(progress.AT_commands_setup[progress.getStage()].getSend());
            progress.setRepeat(0);
            //SerialBT.println("too much repeat");
            return 2;
          }
          Recive(recived);
          deleteRecive();
          Send(progress.AT_commands_setup[progress.getStage()].getSend());
          return 1;
        }
    }
    if(progress.getPart()==2){
      if(AcontainsB(*recived, progress.AT_commands_connect[progress.getStage()].getExpected())){
          progress.incrementStage();
          if(progress.getStage()>=progress.connect_size){
            progress.setPart(3);
            progress.setStage(0);
            progress.setRepeat(0);
            deleteRecive();
            Send(progress.AT_commands_access[0].getSend());
            return 1;
          }
          deleteRecive();
          Send(progress.AT_commands_connect[progress.getStage()].getSend());
        }
        else if(AcontainsB(*recived, progress.AT_commands_connect[progress.getStage()].getErrorMes())){
          progress.setRepeat(progress.getRepeat()+1);
          //SerialBT.println("error code");
          if(progress.getRepeat()>=3){
            for(int i=0;i<progress.AT_commands_connect[progress.getStage()].getStepsBack();i++){
              progress.decrementStage();
              deleteRecive();
              Send(progress.AT_commands_connect[0].getSend());
              return 2;
            }  
          }
          deleteRecive();
          Send(progress.AT_commands_connect[progress.getStage()].getSend());
          return 1;
        }
        else if(millis()-progress.getTimeStamp()>progress.AT_commands_connect[progress.getStage()].getSeconds()*1000){
          progress.setRepeat(progress.getRepeat()+1);
          //SerialBT.println("timeout");
          if(progress.getRepeat()>=5){
            for(int i=0;i<progress.AT_commands_connect[progress.getStage()].getStepsBack();i++){
              progress.decrementStage();
            }
            deleteRecive();
            Send(progress.AT_commands_connect[progress.getStage()].getSend());
            progress.setRepeat(0);
            //SerialBT.println("too much repeat");
            return 2;
          }
          Recive(recived);
          deleteRecive();
          Send(progress.AT_commands_connect[progress.getStage()].getSend());
          return 1;
        }
    }
    if(progress.getPart()==3){
      if(AcontainsB(*recived, progress.AT_commands_access[progress.getStage()].getExpected())){
          progress.incrementStage();
          if(progress.getStage()>=progress.access_size){
            progress.setPart(1);
            progress.setStage(0);
            progress.setRepeat(0);
            deleteRecive();

            //SUCCESS
            timer_ON=millis();
            setLink("");
            return 3;
          }
          deleteRecive();
          if(progress.AT_commands_access[progress.getStage()].getSend()=="LINK"){
            Send("AT+HTTPPARA=\"URL\",\""+link+"\"");
          }
          else{
            Send(progress.AT_commands_access[progress.getStage()].getSend());    
          }
        }
        else if(AcontainsB(*recived, progress.AT_commands_access[progress.getStage()].getErrorMes())){
          progress.setRepeat(progress.getRepeat()+1);
          //SerialBT.println("error code");
          if(progress.getRepeat()>=5){
            for(int i=0;i<progress.AT_commands_access[progress.getStage()].getStepsBack();i++){
              progress.decrementStage();
              deleteRecive();
              Send(progress.AT_commands_access[0].getSend());
              return 2;
            }  
          }
          deleteRecive();
          Send(progress.AT_commands_access[progress.getStage()].getSend());
          return 1;
        }
        else if(millis()-progress.getTimeStamp()>progress.AT_commands_access[progress.getStage()].getSeconds()*1000){
          progress.setRepeat(progress.getRepeat()+1);
          //SerialBT.println("timeout");
          if(progress.getRepeat()>=5){
            for(int i=0;i<progress.AT_commands_access[progress.getStage()].getStepsBack();i++){
              progress.decrementStage();
            }
            deleteRecive();
            Send(progress.AT_commands_access[progress.getStage()].getSend());
            progress.setRepeat(0);
            //SerialBT.println("too much repeat");
            return 2;
          }
          Recive(recived);
          deleteRecive();
          Send(progress.AT_commands_access[progress.getStage()].getSend());
          return 1;
        }
    }
    return 99;
  }
};
