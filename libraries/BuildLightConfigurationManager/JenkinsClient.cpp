#include "JenkinsClient.h"
#include <string.h>
#include <stdlib.h>

uint16_t JenkinsClient::getStatusForJob(JenkinsJob * job, EthernetClient * client) {
  uint16_t combinedDisposition = 0;
  
  for(int i = 0 ; i < MAX_LOCATIONS_PER_LINE ; i++ ){
    if(job->m_jobLocations[i] != NULL){
      uint16_t locationDisposition = getStatusForLocation(job->m_ip, job->m_port, job->m_jobLocations[i], client);
      if(locationDisposition > combinedDisposition ) {
        combinedDisposition = locationDisposition;
      }
    }
  }

#ifdef DEBUG_JENKINS_CLIENT
  Serial.print(F("Combined disposition: "));
  Serial.println(combinedDisposition, BIN);
#endif

  return combinedDisposition;
}

uint16_t JenkinsClient::getStatusForLocation(uint8_t ip[], uint16_t port, char *location, EthernetClient *client){
  uint16_t disposition = 0;

#ifdef DEBUG_JENKINS_CLIENT  
  Serial.print(F("Making request to  IP:"));
  char buffer[16] = {(char)NULL};
  printIp(ip, buffer);
  Serial.print(buffer);
  Serial.println();
#endif

  // if you get a connection, report back via serial:
  if (client->connect(ip, port)) {
#ifdef DEBUG_JENKINS_CLIENT  
    Serial.print(F("connected\n"));
    // Make a HTTP request:
    Serial.print(F("GET "));
    Serial.print(location);
    Serial.println(JENKINS_POST_JOB_URL);
#endif

    client->print("GET ");
    client->print(location);
    client->println(JENKINS_POST_JOB_URL);
  } 
  else {
    // if you didn't get a connection to the server:
#ifdef DEBUG_JENKINS_CLIENT  
    Serial.print(F("connection failed\n"));
#endif
    client->stop();
    return JOB_INVALID_STATUS;
  }
  
  while (!client->available()) {
    //wait
  }
  
  //now read until we encounter the first {
  char tmp = ' ';
  while(client->connected() && tmp != '{') {
  	if(client->available()){
  	  tmp = client->read();
  	}
  }
  
  char status[76] = {'\0'};
  int pos = 0;
  
  //assuming that the _class name won't have a } in it.
  int bytesRead = client->readBytesUntil('}',status,75);
  status[bytesRead] = (char)NULL;
#ifdef DEBUG_JENKINS_CLIENT  
  Serial.println(status);
#endif
  client->flush();
  
#ifdef DEBUG_JENKINS_CLIENT  
  Serial.println();
  Serial.println(F("disconnecting."));
#endif
  client->stop();
  
  char prefix[] = "\"color\":\"";
  char *color = '\0';
  
  if((color=strstr(status, prefix))==0){
    return JOB_INVALID_STATUS;
  }
  
  disposition |= (strstr(color, "disabled") != NULL) ? JOB_DISABLED : 0;
  disposition |= (strstr(color, "blue") != NULL) ? JOB_SUCCEEDED : 0;
  disposition |= (strstr(color, "red") != NULL) ? JOB_FAILED : 0;
  disposition |= (strstr(color, "yellow") != NULL) ? JOB_UNSTABLE : 0;
  disposition |= (strstr(color, "grey") != NULL) ? JOB_CANCELED : 0;
  disposition |= (strstr(color, "aborted") != NULL) ? JOB_CANCELED : 0;
  disposition |= (strstr(color, "anime") != NULL) ? JOB_IN_PROGRESS : 0;
  
#ifdef DEBUG_JENKINS_CLIENT  
  Serial.print(F("Found status: "));
  Serial.println(color);
  Serial.print(F("Mapped to disposition: "));
  Serial.println(disposition, BIN);
#endif

  return disposition;
}

