#ifndef JSONMESSAGE_H
#define JSONMESSAGE_H

/* Wrapper for json structs which contain printer messages */

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <string>
#include <queue>
#include <JsonConfig.h>
#include <Mutex.h>

class PrinterMessage{
	public:
		std::string m_text;
		int m_id;
		PrinterMessage(std::string text, int id):
			m_id(id),
			m_text(text) {
			}
		~PrinterMessage(){}
};

/* Subelement of B9CreatorSettings. Store messages which
 used to communicate between threads.
 (I.e. webserver thread, serial communication thread)
messageQueue: Messages from serial to web
commandQueue: Messages from web to serial
*/
class Messages{
	public:
		std::queue<PrinterMessage> m_messageQueue;
		std::queue<std::string> m_commandQueue;
		Mutex m_messageMutex;
		Mutex m_commandMutex;
		Messages(): 
			m_messageQueue(),
			m_messageMutex(),
			m_commandQueue(),
			m_commandMutex(),
			m_messageId(0),
			m_commandId(0) {

			}
		void add_message(std::string &str){
			m_messageMutex.lock();
			while( m_messageQueue.size() > MAX_QUEUE_SIZE){
				m_messageQueue.pop();
			}
			PrinterMessage pm(str,m_messageId++);
			m_messageQueue.push( pm );
			m_messageMutex.unlock();
		}
		
		void add_command(std::string &str){
			m_commandMutex.lock();
			m_commandQueue.push( str );
			m_commandMutex.unlock();
#ifdef VERBOSE
			std::cout << "Add command " << str << std::endl;
#endif
		}

	private:
		int m_messageId;
		int m_commandId;
		static const int MAX_QUEUE_SIZE=300;
};


static cJSON* jsonMessage(PrinterMessage message){
	cJSON* df = cJSON_CreateObject();
	cJSON_AddItemToObject(df, "text", cJSON_CreateString(message.m_text.c_str()) );
	cJSON_AddItemToObject(df, "line", cJSON_CreateNumber(message.m_id) );//line=id=index
	return df;
};

/*
Structure of return value:
 { 
  "kind" : "B9CreatorMessages",
  "html" : [{
    "type" : "messagesField",
    "id" : "serialMessages",
    "messages" : [{
      "line" : x,
      "text" : "First message"
    },{ 
      "line" : x+1,
      "text" : "Second message"
    }]  
  }]  
}
 */
static cJSON* jsonMessages(const char* id,
		std::queue<PrinterMessage> &messagesQueue
		){

	cJSON* df = cJSON_CreateObject();
	cJSON_AddStringToObject(df, "kind", "B9CreatorMessages");
	cJSON* html = cJSON_CreateArray();	
	cJSON* html_entry1 = cJSON_CreateObject();

	cJSON_AddStringToObject(html_entry1, "type", "messagesField");
	cJSON_AddStringToObject(html_entry1, "id", id);

	cJSON* messages = cJSON_CreateArray();	
	while( messagesQueue.size()>0 ){
		cJSON_AddItemToArray(messages, jsonMessage( messagesQueue.front() ) );
		messagesQueue.pop();
	}

	cJSON_AddItemToObject(html_entry1, "messages", messages);

	cJSON_AddItemToArray(html, html_entry1 );
	cJSON_AddItemToObject(df, "html", html);

	return df;
};

#endif
