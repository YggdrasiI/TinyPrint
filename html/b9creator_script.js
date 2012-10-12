/* Fill in json values on desired
 * positions on the page. Raw values
 * will wrapped by some dynamic html stuff.
*/
function create_fields(json_b9creator){
	var b9creator = JSON.parse(json_b9creator);
	if( b9creator.html !== null )
		for(i=0; i<a.html.length; i++){
			o = b9creator.html[i];
			if( typeof o.caller === 'string'
					&& typeof window['create_'+o.caller] === 'function'
					&& typeof $('#'+o.id) === 'object'
				) {
				(window['create_'+o.caller])(o, $('#'+o.id) );
			}
		}
	//TODO handle com messages
	//if( b9creator.messages !== null ) alert("Todo");

}

/* Take content of json file
 * to update values on the page.
 */
function update_fields(json_b9creator){


}

function create_intField(obj, pnode){
		description = "Id: "+obj.id+", Min: "+obj.min+", Max: "+obj.max;
		ret = $("<span title='"+description+"' alt='"+description+"'>");
		ret.addClass("json_input");
		inputfield = $('<input id="'+obj.id+'" readonly="readonly" value="'+obj.val+'" size="6" />');
		inputfield.onchange( 
				function(event){
					alert(this.value);
					if(check_intField(obj, this.value)){
						//unset red property	
						$("#"+obj.id).removeClass("red");
					}else{
						//mark element red
						$("#"+obj.id).addClass("red");
					}
				});
		//add event for element leave....
		
		ret.append( inputfield ); 
		pnode.append( ret );
}

/* o is subelement of json obj
 * Checks if val
 * */
function check_intField(o, val){
	i = parseInt(val);
	if( ! isFinite(i) ) return false;
	if( i < o.min ) return false;
	if( i > o.max ) return false;
	return true;
 	//val = Math.min(Math.max(min,Number(o.value)+diff),max);
}


function myCheckField(myoptions){
	ret = function(options) {
		// Return a new checkfield
		ret = $("<p>");
		inputfield = $('<input type="checkbox" id="'+options.id+'" value="1234" '+(options.val==1?'checked ':'')+'/>');//.dform('attr', options)
		inputfield.click( function(event){ myoptions.change(options.id,this.checked); });
		propName = $('<span>'+options.id+': </span>');
		propName.addClass("propName");

		ret.append( inputfield ); 
		ret.prepend(propName);
		return  ret;
	}
	return ret;
}


function changeDouble(id,min,max,diff){
	//update displayed value
	o = document.getElementById(id);
 	val = Math.min(Math.max(min,Number(o.value)+diff),max);
	//round to cut rounding errors
	val = Math.round( val*1E6)/1E6;
 	o.value = val;

	//modifyJson obj.
	modifyJson(id,val);

	//send altered obj. to server
	send("json?actionid=0","settingKinect="+JSON.stringify(json_b9creator));
}

function changeInt(id,min,max,diff){
	//update displayed value
	o = document.getElementById(id);
 	val =  Math.min(Math.max(min,Math.round(Number(o.value)+diff)),max);
 	o.value = val;

	//modifyJson obj.
	modifyJson(id,val);

	//send altered obj. to server
	send("json?actionid=0","settingKinect="+JSON.stringify(json_b9creator));
}

function changeCheckbox(id,checked){
	//update displayed value ...
	//... not ness. for checkbox.
	
	//modifyJson obj.
	modifyJson(id,checked?1:0);

	//send altered obj. to server
	send("json?actionid=0","settingKinect="+JSON.stringify(json_b9creator));
}

//modifiy children of html node (no rekursion implemented)
function modifyJson(id,val){
	$.each(	json_b9creator.html, function(index,v){
		if(v.id==id){ v.val=val; }
		});
}

//send complete json struct
function send(url,val){
	$.post(url, val , function(data){
		if( data == "reload" ){
			//alert("Reload Page");
			window.location.reload();
		}else{
			//some other reaction...
		}
	});

	return true;
}

function setView(i){
	send("json?actionid=5","view="+i);
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
