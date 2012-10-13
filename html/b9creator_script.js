/*
 * Identify values (second key) of some
 * entryes (first key) with strings.
 * */
TOKENS = {
	"projectorStatus" : {0 : "Off" , 1 : "On" },
	"printerStatus" : {0 : "Not ready" , 1 : "Ready", 2 : "Error" },
}


/* Fill in json values on desired
 * positions on the page. Raw values
 * will wrapped by some dynamic html stuff.
*/
function create_fields(json_b9creator){
	cu_fields(json_b9creator,"create");
	//TODO handle com messages
	//if( b9creator.messages !== null ) alert("Todo");
}

/* Take content of json file
 * to update values on the page.
 */
function update_fields(json_b9creator){
	cu_fields(json_b9creator,"update");
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
		inputfield.attr("prevvalue", val);
		
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
						inputfield.attr("prevvalue", this.value);
						o.val = parse(o,this.value);
						refresh();
					}else{
						//reset value.
						this.value = inputfield.attr("prevvalue");
						pnode.removeClass("red");
					}
				});

		ret.append( inputfield ); 
		pnode.append( ret );
}

function update_intField(obj){
	var val = format(obj,obj.val);
	var inputfield = $("#"+obj.id+"_"); 
	inputfield.val(val);
	inputfield.attr("prevvalue", val);
//	inputfield.trigger('input');
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
		inputfield.attr("prevvalue", val);
		
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
						inputfield.attr("prevvalue", this.value);
						o.val = parse(o,this.value);
						refresh();
					}else{
						//reset value.
						this.value = inputfield.attr("prevvalue");
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
	var statefield = $(obj.id+"_"); 
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
		
		//add toggle event
		inputfield.change( function(event){
			var o = pnode.prop("json"); 
			o.val = ( $(this).prop("checked")!=false?1:0 );
			json_b9creator.html[0].val ++;
			refresh();
		});

		ret.append( inputfield ); 
		pnode.append( ret );
}

function update_checkboxField(obj){
	$("#"+obj.id+"_").prop("checked", obj.val!=0 );
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
	var p = (typeof val === "string"?parseInt(val)*100:val*100);
	return p+"%";
}

function parse_percent(o,s){
	return parseFloat(s)/100;
}

/* millimeter, similar to percent. Just for testing. It's no good idea to use 1m as base measure... */
function format_mm(o,val){
	var p = (typeof val === "string"?parseInt(val)*1000:val*1000);
	return p+"mm";
}

function parse_mm(s){
	return parseFloat(s)/1000;
}

/* Send current json struct to server,
 * get response and refresh displayed 
 * values */
function refresh(){
	send("json?actionid=0","b9CreatorSetting="+JSON.stringify(json_b9creator),
			function(data){
				json_b9creator = JSON.parse(data);//change global var
				update_fields(json_b9creator);
			}
			);
}

//send complete json struct
function send(url,val, handler){
	$.post(url, val , function(data){
		//alert("Get reply\n"+data);
		if( data == "reload" ){
			//alert("Reload Page");
			window.location.reload();
		}else{
			//reparse data
			if( handler != null ) handler(data);
		}
	});

	return true;
}

function setView(i){
	send("json?actionid=5","view="+i);
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
		
