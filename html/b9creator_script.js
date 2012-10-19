/*
 * Identify values (second key) of some
 * entryes (first key) with strings.
 * */
TOKENS = {
	"projectorStatus" : {2 : "?" , 0 : "Off" , 1 : "On" },
	"resetStatus" : {0 : "Not required." , 1 : "Required.", 2 : "Error" },
}

/* maximal number of lines in the messages 
 * textarea.
 * */
TEXTAREA_MAX_LINES = 300;


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
					alert("Can not "+prefix+" field "+o.id+".");
				}
		}
}


function create_intField(obj, pnode){
	var description = "Id: "+obj.id+", Min: "+obj.min+", Max: "+obj.max;
	var ret = $("<span title='"+description+"' alt='"+description+"'>");
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
	var ret = $("<span title='"+description+"' alt='"+description+"'>");
	ret.addClass("json_input");

	//format value
	var val = format(obj,obj.val);

	var inputfield = $('<input id="'+obj.id+'_" value="'+val+'" size="6" />');
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
	var ret = $("<span title='"+description+"' alt='"+description+"'>");
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
	var ret = $("<span title='"+description+"' alt='"+description+"'>");
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
	var ret = $("<span title='"+description+"' alt='"+description+"'>");
	ret.addClass("json_input");

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



/**
 * Format functions.
 * Use functions with the name pattern "format_[style]" to convert
 * json values in strings.
 * Use functions with the name pattern "parse_[style]" to invert
 * the operation. Readonly values do not require parse functionality.
 * */
/*identities */
function format_(o,val){ return val; }
function parse_(o,s){ p=parseFloat(s); return isNaN(p)?s:p; }

/* Use format_token to generate string */
function format_token(o,val){
	var arr = TOKENS[o.id];
	if( val in arr ) return arr[val];
	return "undefined";
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
	var p = (typeof val === "string"?parseInt(val):val);
	return p+"mm";
}

function parse_mm(s){
	//return parseFloat(s)/1000;
	return parseFloat(s);
}

/* Send current json struct to server and refresh displayed values.
*/
function send_setting(){
	send("update?actionid=0","b9CreatorSetting="+JSON.stringify(json_b9creator), null);

	if(false)
		send("json","",
				function(data){
					json_b9creator = JSON.parse(data);//change global var
					update_fields(json_b9creator);
				}
				);
}

/*
 * refresh raw message window */
function refresh(){
	send("json","",
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

}

//send complete json struct
function send(url,val, handler){
	//Add space to avoid empty second arg!
	$.post(url, val+" ", function(data){
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
	/* With post arg display=0: display off.
	 * With post arg display=1: display on.
	 * With post arg display=2: display toggle.
	 * Without post arg: Just get value.
	 * Return value data: display status (0|1).
	 * */
	send("update?actionid=5","display=2",
			function(data){
				if( data == 1 ){
					button.value = "Hide Display"
				}else{
					button.value = "Show Display"
				}
			}
			);
}


function jobManager(button,print,){
	/* print="start": start print
	 * print=="pause" : pause print
	 * ToDo:
	 * (print=="abort": stop print
	 * print="toggle": toggle print)
	 */
	var map = {"abort":0,"start":1,"toggle":2,"pause":3};
	send("update?actionid=6","print="+map[print],
			function(data){
				if( data == 1 ){
					button.value = "Pause" /*printing...*/
				}else if(data == 2){
					button.value = "Resume"/*paused...*/
				}else{
					button.value = "Print"/*stoped...*/
				}
			}
			);
}

function quitServerApp(){
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

