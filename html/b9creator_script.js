/*
 * Identify values (second key) of some
 * entryes (first key) with strings.
 * */
TOKENS = {
	"projectorStatus" : {2 : "?" , 0 : "Off" , 1 : "On" },
	"resetStatus" : {0 : "Not required." , 1 : "Required.", 2 : "Error" },
	"displayStatus" : {0 : "Off." , 1 : "On." },

	//s/\([^ \t]*\)=\([^,]*\)\(,*\)/\2 : "\1"\3
	"jobState" : {
		0 : "RESET",
		1 : "INIT",
		2 : "FIRST_LAYER",
		4 : "NEXT_LAYER",
		8 : "OVERCURING",
		16 : "BREATH",
		32 : "WAIT_ON_F_MESS",
		64 : "WAIT_ON_R_MESS",
		128 : "IDLE",
		256 : "PAUSE",
		512 : "FINISH",
		1024 : "CURING",
		2048 : "START_STATE",
		4096 : "ERROR",
		8192 : "WAIT_ON_ZERO_HEIGHT"
	},

	/* Label and Description for some property names */
	"props" : {
		"maxLayer" : { "label" : "Max Layer:", "desc" : "Lower this value to cut of slices." },
		"minLayer" : { "label" : "Min Layer:", "desc" : "Raiser this value to cut of slices." },
		"positionX" : { "label" : "Horizontal position:", "desc" : "" },
		"positionY" : { "label" : "Vertical position:", "desc" : "" },
		"scale" : { "label" : "Scale factor (xy):", "desc" : "Scale of vector based svg image.\nAttention! This scale will NOT applied in z-Direction." }
	}
}

/* maximal number of lines in the messages
 * textarea.
 * */
TEXTAREA_MAX_LINES = 300;

/* Backup of last state. Used in format_state */
last_state = -1;
/* Flag enable prompts to avoid missclicks on
 * the webinterface during printing */
request_confirmation = false;

/* Fill in json values on desired
 * positions on the page. Raw values
 * will wrapped by some dynamic html stuff.
 */
function create_fields(json){
	cu_fields(json,"create");
}

/* Take content of json file
 * to update values on the page.
 */
function update_fields(json){
	cu_fields(json,"update");
}

function cu_fields(obj,prefix){
	//convert input arg to object, if string representation given.
	//var obj = (typeof json_obj === "string"?JSON.parse(json_obj):json_obj);

	if( typeof obj.html === "object" )
		for(var i=0; i<obj.html.length; i++){
			var o = obj.html[i];
			if( typeof o.type === "string"  /* type=intField, doubleField,... */
					&& typeof window[prefix+"_"+o.type] === "function"
					&& typeof $("#"+o.id) === "object"
				) {
					/* Connect (new) json obj with the container element.
					 * Attention, if you create some event handlers in the
					 *	create_[...] functions do not depend on the object
					 *	'o'. Use $("#"+id+).prop("json").
					 *	Reparsing of json string creates new objects!
					 */
					pnode = $("#"+o.id);
					pnode.prop("json", o);
					(window[prefix+"_"+o.type])(o, pnode );
				}else{
					//alert("Can not "+prefix+" field "+o.id+".");
				}
		}
}


function create_intField(obj, pnode){
	var description = "Id: "+obj.id+", Min: "+obj.min+", Max: "+obj.max;
	//var description = description + " Diff: "+obj.diff;
	var ret = $("<div title='"+description+"' alt='"+description+"'>");
	ret.addClass("json_input");

	//format value
	var val = format(obj,obj.val);

	var inputfield = $('<input id="'+obj.id+'_" value="'+val
			+(obj.readonly==0?'" ':'" readonly="readonly"')
			+'" size="6" />');

	if( obj.readonly!=0 ) $("#"+obj.id).addClass("readonly");

	// prevvalue property: backup of value to compare on changements.
	inputfield.prop("prevvalue", val);

	/* jQuery: .change fires if element lost focus. .input fires on every change. */
	inputfield.bind('input',
			function(event){
				if(check_intField(pnode.prop("json"), this.value)){
					//unset red property
					pnode.removeClass("red");
				}else{
					//mark element red
					pnode.addClass("red");
				}
			});
	//add event for element leave....
	inputfield.change( function(event){
		var o = pnode.prop("json");
		if(check_intField(o, this.value)){
			inputfield.prop("prevvalue", this.value);
			o.val = parse(o,this.value);
			send_setting();
		}else{
			//reset value.
			this.value = inputfield.prop("prevvalue");
			pnode.removeClass("red");
		}
	});

	inputfield.hover( function(event){
		this.focus();
	},
	function(event){
		//this.blur();//unfocus element

		var o = pnode.prop("json");
		if( o.val != parse(o,this.value) ) inputfield.trigger('change');
	});

	inputfield.bind('mousewheel', function(event, delta) {

		var dir = delta>0?1:-1;
		var o = pnode.prop("json");
		if(o.readonly) return false;

		var prevVal = parse(o,this.value);
		var nextVal = prevVal + parseInt(dir*o.diff);

		//cut low and high values
		nextVal = o.min>nextVal ? o.min:((nextVal<o.max)?nextVal:o.max);

		var nextValStr = format(o,nextVal);

		//check if new value is valid.
		if(check_intField(o, nextValStr)){
			this.value = nextValStr;
		}

		return false;
	});

	inputfield.bind('keyup', function(event) {

		var o = pnode.prop("json");
		if(o.readonly) return false;

		var accel =0;
		switch( event.which ){
			case 37: /*Left key */
				accel =-1;
				break;
			case 39: /*Right key */
				accel =1;
				break;
			case 38: /*Up key */
				accel =10;
				break;
			case 40: /*Down key */
				accel =-10;
				break;
			case 33: /*PageUp key */
				accel =10000;
				break;
			case 34: /*PageDown key */
				accel =-10000;
				break;
			case 13: /* Enter */
				inputfield.blur();
				return false;
				break;
			default:
				return false;
				break;
		}


		var prevVal = parse(o,this.value);
		var nextVal = prevVal + parseInt(accel*o.diff);

		//cut low and high values
		nextVal = o.min>nextVal ? o.min:((nextVal<o.max)?nextVal:o.max);

		var nextValStr = format(o,nextVal);

		//check if new value is valid.
		if(check_intField(o, nextValStr)){
			this.value = nextValStr;
			//send updated values to server
			var o = pnode.prop("json");
			if( o.val != nextVal ) inputfield.trigger('change');
		}

		return false;
	});


	ret.append( inputfield );
	pnode.append( ret );
}

function update_intField(obj){
	var val = format(obj,obj.val);
	var inputfield = $("#"+obj.id+"_");
	var prev = inputfield.prop("prevvalue");
	/* This check omit the automaticly change of field
	 * which currently changed by the user.
	 */
	if( val != prev && prev == inputfield.val() ){
		inputfield.val(val);
		inputfield.prop("prevvalue", val);
			inputfield.trigger('input');
	}
	//set readonly flag
	var ro = obj.readonly;
	inputfield.prop("readonly", ro );
	if( ro ) $("#"+obj.id).addClass("readonly");
	else $("#"+obj.id).removeClass("readonly");
}

/* o is subelement of json obj
 * Checks if val
 * */
function check_intField(o, val){
	val = parse(o,val);

	var i = parseInt(val);
	if( ! isFinite(i) ) return false;
	if( i < o.min ) return false;
	if( i > o.max ) return false;
	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

function create_doubleField(obj, pnode){
	var description = "Id: "+obj.id+", Min: "+obj.min+", Max: "+obj.max;
	//var description = description + " Diff: "+obj.diff;
	var ret = $("<div title='"+description+"' alt='"+description+"'>");
	ret.addClass("json_input");

	//format value
	var val = format(obj,obj.val);

	var inputfield = $('<input id="'+obj.id+'_" value="'+val
			+(obj.readonly==0?'" ':'" readonly="readonly"')
			+'" size="6" />');

	if( obj.readonly!=0 ) $("#"+obj.id).addClass("readonly");

	// prevvalue property: backup of value to compare on changements.
	inputfield.prop("prevvalue", val);

	/* jQuery: .change fires if element lost focus. .input fires on every change. */
	inputfield.bind('input',
			function(event){
				if(check_doubleField(pnode.prop("json"), this.value)){
					//unset red property
					pnode.removeClass("red");
				}else{
					//mark element red
					pnode.addClass("red");
				}
			});
	//add event for element leave....
	inputfield.change( function(event){
		var o = pnode.prop("json");
		if(check_doubleField(o, this.value)){
			inputfield.prop("prevvalue", this.value);
			o.val = parse(o,this.value);
			send_setting();
		}else{
			//reset value.
			this.value = inputfield.prop("prevvalue");
			pnode.removeClass("red");
		}
	});

	inputfield.hover( function(event){
		this.focus();
	},
	function(event){
		//this.blur();//unfocus element

		var o = pnode.prop("json");
		if( o.val != parse(o,this.value) ) inputfield.trigger('change');
	});

	inputfield.bind('mousewheel', function(event, delta) {

		var dir = delta>0?1:-1;
		var o = pnode.prop("json");
		if(o.readonly) return false;

		var prevVal = parse(o,this.value);
		var nextVal = Math.round( 100*(prevVal + dir*o.diff) )/100;

		//cut low and high values
		nextVal = o.min>nextVal ? o.min:((nextVal<o.max)?nextVal:o.max);

		var nextValStr = format(o,nextVal);

		//check if new value is valid.
		if(check_doubleField(o, nextValStr)){
			this.value = nextValStr;
		}

		return false;
	});

	inputfield.bind('keyup', function(event) {

		var o = pnode.prop("json");
		if(o.readonly) return false;

		var accel =0;
		switch( event.which ){
			case 37: /*Left key */
				accel =-1;
				break;
			case 39: /*Right key */
				accel =1;
				break;
			case 38: /*Up key */
				accel =10;
				break;
			case 40: /*Down key */
				accel =-10;
				break;
			case 33: /*PageUp key */
				accel =10000;
				break;
			case 34: /*PageDown key */
				accel =-10000;
				break;
			case 13: /* Enter */
				inputfield.blur();
				return false;
				break;
			default:
				return false;
				break;
		}

		var prevVal = parse(o,this.value);
		var nextVal = Math.round( 100*(prevVal + accel*o.diff) )/100;

		//cut low and high values
		nextVal = o.min>nextVal ? o.min:((nextVal<o.max)?nextVal:o.max);

		var nextValStr = format(o,nextVal);

		//check if new value is valid.
		if(check_doubleField(o, nextValStr)){
			this.value = nextValStr;
			//send updated values to server
			var o = pnode.prop("json");
			if( o.val != nextVal ) inputfield.trigger('change');
		}

		return false;
	});


	ret.append( inputfield );
	pnode.append( ret );

}

function update_doubleField(obj){
	update_intField(obj);
}

function check_doubleField(o, val){
	val = parse(o,val);

	var i = parseFloat(val);
	if( ! isFinite(i) ) return false;
	if( i < o.min ) return false;
	if( i > o.max ) return false;
	return true;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/* Simple label field */
function create_stateField(obj, pnode){
	var description = "Id: "+obj.id+" Val: "+obj.val;
	var ret = $("<div title='"+description+"' alt='"+description+"'>");
	ret.addClass("json_input");

	//format value
	var val = format(obj,obj.val);
	var statefield = $('<span id="'+obj.id+'_">'+val+'</span>');

	ret.append( statefield );
	pnode.append( ret );
}

function update_stateField(obj){
	var statefield = $("#"+obj.id+"_");
	var val = format(obj,obj.val);
	var statefield2 = $('<span id="'+obj.id+'_">'+val+'</span>');
	statefield.replaceWith(statefield2);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++


function create_checkboxField(obj, pnode){
	var description = "Id: "+obj.id;
	var ret = $("<div title='"+description+"' alt='"+description+"'>");
	ret.addClass("json_input");

	var inputfield = $('<input type="checkbox" id="'+obj.id+'_" value="yes" />');
	inputfield.prop("checked",obj.val!=0);
	inputfield.prop("prevvalue", obj.val!=0 );

	//add toggle event
	inputfield.change( function(event){
		var o = pnode.prop("json");
		o.val = ( $(this).prop("checked")!=false?1:0 );
		send_setting();
	});

	ret.append( inputfield );
	pnode.append( ret );
}

function update_checkboxField(obj){
	var val = obj.val!=0;
	var checkbox = $("#"+obj.id+"_");
	var prev = checkbox.prop("prevvalue");
	if( val != prev && prev == checkbox.prop("checked") ){
		checkbox.prop("checked", obj.val!=0 );
		checkbox.prop("prevvalue", obj.val);
	}
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++

function create_messagesField(obj, pnode){
	var description = "Messages field. Refresh every second. Id: "+obj.id;
	var ret = $("<div title='"+description+"' alt='"+description+"'>");
	//ret.addClass("json_input");

	var field = $('<textarea id="'+obj.id+'_" rows="10" cols="35" >');
	field.prop("readonly",true);
	field.addClass("readonly");

	var messarr = new Array();
	field.prop("messarr",messarr);

	//insert messages
	$.each(obj.messages, function(index, value){
		//field.val(field.val()+value.line+" "+value.text+"\n");
		messarr.push( value.line+" "+value.text );
	});
	field.val( messarr.join("\n") );

	//scroll to bottom
	field.scrollTop(field.scrollHeight);

	ret.append( field );
	pnode.append( ret );
}

function update_messagesField(obj){
	var field = $("#"+obj.id+"_");

	var messarr = field.prop("messarr");
	var scrollPos = field[0].scrollTop;
	var onBottom = (scrollPos+field.height() + 30 > field[0].scrollHeight );

	$.each(obj.messages, function(index, value){
		//field.val(field.val()+value.line+" "+value.text+"\n");
		messarr.push( value.line+" "+value.text );
	});

	//remove old enties
	while( messarr.length > TEXTAREA_MAX_LINES ){
		messarr.shift(); //remove first entry
	}

	field.val( messarr.join("\n") );
	field.prop("messarr",messarr);
	if( onBottom ){
		//scroll to bottom
		field.scrollTop(field[0].scrollHeight);
	}else{
		//scroll to last known position. This is not perfect if old lines was removed.
		field.scrollTop(scrollPos);
	}
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++

function create_filesField(obj, pnode){
	var description = "List with all open files.";
	var ret = $("<div title='"+description+"' alt='"+description+"'>");
	//ret.addClass("json_input");
	pnode.append(ret);

	//loop throuth files-Array.
	for( var i in obj.filearray){
		var file = obj.filearray[i];
		var filespan = $("<div tite='"+file.filename+"' id='file_"+i+"'>");
		filespan.append( $("<h2>"+file.filename+" [<a href='javascript:unloadFile("+i+")'>X</a>]</h2>") );
		filespan.append( $("<p>"+file.description+"</p>") );
		ret.append(filespan);

		//create elements with the right ids.
		for( var j in file.html ){
			var prop = file.html[j];
			//mod id to get unique values. Removed and now set on server side
			//prop.id = "file"+i+"_"+prop.id;
			var propType = prop.id.substr( prop.id.indexOf("_")+1 );
			var text = TOKENS["props"][propType]["label"];
			var desc = TOKENS["props"][propType]["desc"];
			var propspan = $("<div id='"+prop.id+"' title='"+desc+"'>"+text+" </div>");
			propspan.addClass("prop");
			filespan.append(propspan);
		}

		//fill new element nodes
		create_fields(file);

	}

}

function update_filesField(obj){

	//loop throuth files-Array.
	for( var i in obj.filearray){
		var file = obj.filearray[i];

		//fill new element nodes
		update_fields(file);
	}
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++

/*
 * Convert json_files into drop down list
 */
function create_jobFileList(){
	pnode = $("#fileBrowserList");

	var description = "List of job files. Edit the *.ini file to change the folder";
	var ret = $("<div title='"+description+"' alt='"+description+"'>");
	ret.addClass("json_input");

	var selectfield = $('<select id="fileBrowserListSelection" size="1">');
	selectfield.change( function(event){
		//alert($(this).val());
	});

	ret.append( selectfield );
	pnode.append( ret );

	filling_jobFileList( "fileBrowserListSelection" , json_job_files.content );
}

function filling_jobFileList( id, objArr ){
	selectfield =  $('#'+id);
	$('#'+id+" option").remove();
	if( objArr.length < 1 ){
		$("<option/>").val("-1").text("No files.").appendTo(selectfield);
	}else{
		for(var i=0; i<objArr.length; i++){
			$("<option/>").val(objArr[i][""+i+""]).text(objArr[i][i]).appendTo(selectfield);
		}
	}
}

/*
 * Update files list.
 * */
function update_jobFileList(){
	send("files","",
			function(data){
				json_job_files = JSON.parse(data);//change global var
				filling_jobFileList( "fileBrowserListSelection" , json_job_files.content );
			}
			);
}

/*
 *
 * */
function loadFile(button){

	var filename = $('#fileBrowserListSelection').val();
	var pnode = $('#files');

	//block load button
	$(button).prop("disabled",true);


	send("update?actionid=7","job_file="+filename,
			function(data){
				if( "failed" != data ){
					var job_files = JSON.parse(data);//local var.
					pnode.empty();
					create_filesField( job_files.html[0], pnode);
				}
				else alert("Loading of file failed.\nReturn value of server:\n"+data);
				$(button).prop("disabled",false);
			}
			);
}


function create_jobTimingStates(){
	/*
		 send("jobtimings","",
		 function(data){
		 var job_timings = JSON.parse(data);
		 create_fields(job_timings);
		 }
		 );
		*/
	//now already readed in job_timings.js
	create_fields(job_timings);
}


function unloadFile(fileindex){
	//its possible to load the same job twice.
	//thus, we do not use the filename to identify files.
	var ret = confirm("Remove file?");
	if( ret ){
		var pnode = $('#files');
		send("update?actionid=8","job_file_index="+fileindex,
				function(data){
					if( "failed" != data ){
						var job_files = JSON.parse(data);
						pnode.empty();
						create_filesField( job_files.html[0], pnode);
					}
				}
				);
	}
}


function loadConfig(){

	ok = !request_confirmation || confirm("Printing... Are you sure?");
	if( !ok ) return;
	
	var configFilename = $('#configFilename').val();
	send("update?actionid=1","configFilename="+configFilename,
			function(data){
				if( data == "ok" ) window.location.reload();
				else /*if( data == "error" )*/ alert("Loading failed");
			}
			);
}

function saveConfig(){
	var configFilename = $('#configFilename').val();
	send("update?actionid=2","configFilename="+configFilename,
			function(data){
				if( data == "error" ) alert("Saveing failed");
			}
			);
}

function resetPrinter(){
	ok = !request_confirmation || confirm("Printing... Are you sure?");
	if( !ok ) return;

	sendCmd('R');
}

/**
 * Format functions.
 * Use functions with the name pattern "format_[style]" to convert
 * json values in strings.
 * Use functions with the name pattern "parse_[style]" to invert
 * the operation. Readonly values do not require parse functionality.
 * */
/*identities */
function format_(o,val){ return ""+val; }
function parse_(o,s){ p=parseFloat(s); return isNaN(p)?s:p; }

/* Use format_token to generate string */
function format_token(o,val){
	var arr = TOKENS[o.id];
	if( val in arr ) return arr[val];
	return "undefined";

}

/* format_state will be called for
 * job state string. Analyse this string
 * and update Print buttons. */
function format_state(o,val){
	state = format_token(o,val);

	if( val == last_state ){
		return state;
	}

	switch( state ){
		case "START_STATE":
//		case "INIT":
			$('#printButton').val("Print (Init)");
			break;
		case "PAUSE":
			$('#printButton').val("Resume");
			break;
		case "IDLE":
			request_confirmation = false;
			$('#printButton').val("Print");
			$('#openButton').prop("disabled",false);
			//$('#resetButton').prop("disabled",false);
			break;
		case "BREATH":
		case "CURING":
		case "OVERCURING":
		case "WAIT_ON_ZERO_HEIGHT":
			request_confirmation = true;
			$('#printButton').val("Pause");
			$('#openButton').prop("disabled",true);
			//$('#resetButton').prop("disabled",true);
			break;
	}

	return state;
}


function format_percent(o,val){
	var p = (typeof val === "string"?parseInt(val):val);
	if( p == -100 ) return "?%";
	return p+"%";
}

function parse_percent(o,s){
	if( s == "?%" ) return -100;
	return parseFloat(s);
}

/* millimeter, similar to percent. */
function format_mm(o,val){
	//var p = (typeof val === "string"?parseInt(val)*1000:val*1000);
	var p = (typeof val === "string"?parseInt(val)/100:val/100);
	var p = Math.round(p*100)/100;
	return p+"mm";
}

function parse_mm(s){
	//return parseFloat(s)/1000;
	return parseFloat(s)*100;
}

/* Parse seconds into hh:mm:ss format */
function format_hhmmss(o,val){
	var p = (typeof val === "string"?parseInt(val):val);
	if( p < 0 ) return "00:00:00";
	s = p%60;
	p = Math.floor(p/60);
	m = p%60;
	h = Math.floor(p/60);
//	return sprintf("%2.0f:2.0f:2.0f", h,m,s);
 return (h<10?"0":"") + h + (m<10?":0":":") + m + (s<10?":0":":") + s;
}

/* Send current json struct to server and refresh displayed values.
*/
function send_setting(){
	//l = json_b9creator["html"].length;
	//alert(JSON.stringify (json_b9creator["html"][l-1]["filearray"] ));
	//alert(JSON.stringify (json_b9creator ));
	send("update?actionid=0","b9CreatorSettings="+JSON.stringify(json_b9creator), null);

	if(false)
		send("settings","",
				function(data){
					json_b9creator = JSON.parse(data);//change global var
					update_fields(json_b9creator);
				}
				);
}

/*
 * refresh raw message window */
function refresh(){
	send("settings","",
			function(data){
				json_b9creator = JSON.parse(data);//change global var
				update_fields(json_b9creator);
			}
			);

	send("messages","",
			function(data){
				json_messages = JSON.parse(data);//change global var
				update_fields(json_messages);
			}
			);

	send("jobtimings","",
			function(data){
				var job_timings = JSON.parse(data);
				update_fields(job_timings);
			}
			);
}

//send complete json struct
function send(url,val, handler){
	//Add space to avoid empty second arg!
	if(val == "") val = " ";
	$.post(url, val, function(data){
		//alert("Get reply\n"+data);
		if( data == "reload" ){
			alert("Reload Page");
			window.location.reload();
		}else{
			//reparse data
			if( handler != null ) handler(data);
		}
	});

	return true;
}


function toggleDisplay(button){

	ok = !request_confirmation || confirm("Printing... Are you sure?");
	if( !ok ) return;

	/* With post arg display=0: display off.
	 * With post arg display=1: display on.
	 * With post arg display=2: display toggle.
	 * Without post arg: Just get value.
	 * Return value data: display status (0|1).
	 * */
	send("update?actionid=5","display=2",
			function(data){
				return;
				if( data == 1 ){
					button.value = "Hide Display"
				}else{
					button.value = "Show Display"
				}
			}
			);
}


/* Start, stop or pause printing job
 * Require two buttons and update the button labels.
 * Possible commands:
	 * print="init": init printer (required for start)
	 * print="start": start print
	 * print=="pause" : pause print
	 * print="toggle": toggle print
	 * print=="abort": stop print
 * */
function jobManagerCmd(cmd,button1id, button2id){
	//var map = {"init":0,"start":1,"toggle":2,"pause":3,"abort":4};
	var bt1 = $("#"+button1id);
	var bt2 = $("#"+button2id);
	//send("update?actionid=6","print="+map[cmd],
	send("update?actionid=6","print="+cmd,
			function(data){
				//It's not ness to listen on return value anymore.
				//The labels of the buttons will updated by jobState value.
				return;
				/* data return state of printer */
				if( data == "print" ){
					/*printing...*/
					bt1.val("Pause");
					bt2.val("Abort");
				}else if(data == "pause"){
					/*paused...*/
					bt1.val("Resume");
					bt2.val("Abort");
				}else if(data == "idle"){
					/*stoped/idle...*/
					bt1.val("Print");
					bt2.val("Abort");
				}else{
					bt1.val("Command was not sucessfull.\n Server returns: \n"+data);
				}
			}
			);
}

/* Abort printing job. Same structure as jobManagerPrint.*/
function goToFillLevel(){
	ok = !request_confirmation || confirm("Printing... Are you sure?");
	if( ok )
		sendCmd('G630')
}


function quitServerApp(){
	ok = !request_confirmation || confirm("Quit TinyPrint Server?");
	if( ok )
		send("update?actionid=3","",null);
}

/* Send serial command */
function sendCmd(str){
	send("update?actionid=4","cmd="+str,null);
}

function format(o,val){
	if( typeof window["format_"+o.format] === "function" )
		return (window["format_"+o.format])(o,val);
	return val;
}

function parse(o,s){
	if( typeof window["parse_"+o.parse] === "function" )
		return (window["parse_"+o.parse])(o,s);
	return s;
}




//modifiy children of html node (no rekursion implemented)
function modifyJson(id,val){
	$.each(	json_b9creator.html, function(index,v){
		if(v.id==id){ v.val=val; }
	});
}


function deepCopy(p,c) {
	var c = c||{}; for (var i in p) {
		if (typeof p[i] === 'object') {
			c[i] = (p[i].constructor === Array)?[]:{};
			deepCopy(p[i],c[i]);
		} else c[i] = p[i];
	}
	return c;
}

