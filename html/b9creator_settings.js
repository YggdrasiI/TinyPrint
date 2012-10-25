json_b9creator = {{ONION_JSON}};    

json_messages = {
	"kind" : "B9CreatorMessages",
	"html" : [{
		"type" : "messagesField",
		"id" : "serialMessages",
		"messages" : []
	}]
};


$(function(){
		/*Add hidden input field stub and create form*/
		/*
			 a = deepCopy(json_kinect);
			 b ={"name":"json","id":"json","type":"text","size":"100","value":JSON.stringify(json_kinect)};
			 a.html[a.html.length] = b;
				//$("#settingKinectForm").dform(a);
		*/

		create_fields(json_b9creator);
		create_fields(json_messages);

		create_jobFileList();

		refreshIntervall = window.setInterval("refresh()", 1000);
});

