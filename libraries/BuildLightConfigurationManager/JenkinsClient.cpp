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

  char ip_string[16] = {(char)NULL};
  printIp(ip, ip_string);

#ifdef DEBUG_JENKINS_CLIENT  
  Serial.print(F("Making request to IP:"));
  Serial.print(ip_string);
  Serial.print(F(":"));
  Serial.print(port);
  Serial.println();
#endif

  // if you get a connection, report back via serial:
  if (client->connect(ip, port)) {
#ifdef DEBUG_JENKINS_CLIENT  
    Serial.print(F("connected\n"));
    // Make a HTTP request:
    Serial.print(F("GET "));
    Serial.print(location);
    Serial.print(JENKINS_POST_JOB_URL);
    Serial.println(F(" HTTP/1.0"));
    Serial.print(F("Host: "));
    Serial.print(ip_string);
    Serial.print(F(":"));
    Serial.println(port);
    Serial.println(F("User-Agent: Arduino"));
    Serial.println(F("Accept: */*"));
#endif

    client->print("GET ");
    client->print(location);
    client->print(JENKINS_POST_JOB_URL);
    client->println(" HTTP/1.0");
    client->print(F("Host: "));
    client->print(ip_string);
    client->print(F(":"));
    client->println(port);
    client->println(F("User-Agent: Arduino"));
    client->println(F("Accept: */*"));
    client->println();
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
  
  //We are expecting something like this on the wire:
/*
HTTP/1.1 200 OK
Date: Sun, 15 Apr 2018 14:12:35 GMT
X-Content-Type-Options: nosniff
Set-Cookie: JSESSIONID.90b3d1dd=node0am2eegk7zxku11bqkqg8j2gse47.node0;Path=/;HttpOnly
Expires: Thu, 01 Jan 1970 00:00:00 GMT
X-Jenkins: 2.112
X-Jenkins-Session: 50216c0f
Content-Type: application/json;charset=utf-8
Content-Length: 57
Server: Jetty(9.4.z-SNAPSHOT)

{"_class":"hudson.model.FreeStyleProject","color":"blue"}
*/

  //now read until we encounter the first {
  char tmp = ' ';
  while(client->connected() && tmp != '{') {
  	if(client->available()){
  	  tmp = client->read();
#ifdef DEBUG_JENKINS_CLIENT  
      Serial.print(tmp);
#endif
  	}
  }
  
  char status[76] = {'\0'};
  int pos = 0;
  
  //assuming that the _class name won't have a } in it.
  int bytesRead = client->readBytesUntil('}',status,75);
  status[bytesRead] = (char)NULL;
#ifdef DEBUG_JENKINS_CLIENT
  Serial.print(F("status: "));
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

