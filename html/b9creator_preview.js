function utf8_to_b64( str ){
	return window.btoa(unescape(encodeURIComponent( str )));
}

function b64_to_utf8( str ){
	return decodeURIComponent(escape(window.atob( str )));
}

function reloadImage(manual){

	if(manual){
		//clear pending refresh
		window.clearInterval(refreshIntervall);
	}

	if( !manual && $("#previewRefresh").prop("checked") != true ) return true;

	//var scale = $("#previewScale").val();//to early change
	var scale = $("#previewScale").prop("zoomval");
	if( isNaN(scale) ) scale = 50;
	d = new Date();

	/* //Does not  work. Input has wrong coding
		 send("preview.png","scale="+scale,
		 function(data){
		 if( data == "noNewImage" ){
		 $("#info").empty().fadeIn(0).append("No new image").fadeOut(1000);
		 }else{
		 $("#previewImg").attr("src", "");
	//$("#info").empty().fadeIn(0).append("Load new image").fadeOut(1000);
	$("#info").empty().append( utf8_to_b64(data) );
	//	$("#previewImg").attr("src", "data:image/png;base64,"+utf8_to_b64(data)+"=" );
	//$("#previewImg").attr("src", "data:image/png;base64,"+data );
	//$("#previewImg").attr("src", "data:image/png,"+encodeURI(data) );
	//$("#previewImg").attr("src", "data:image/png;base64,"+encodeURI(data) );
	} });
	 */

	var newImg = new Image();
	newImg.onload = function(){
		if( newImg.width < 2 ){
			if(manual) $("#info").empty().fadeIn(0).append("No new image").fadeOut(1000);
		}else{
			if(manual) $("#info").empty().fadeIn(0).append("Load new image").fadeOut(1000);
			$("#previewImg").empty().append($(newImg));
		}
	};

	newImg.src = "preview.png?scale="+scale+"#"+d.getTime();

	if(manual){
		refreshIntervall = window.setInterval("reloadImage(false)", 500);
	}

	return true;
}


$(function(){

//add event handler to update zoom factor
	scaleSelect = $("#previewScale");
	scaleSelect.prop("zoomval", scaleSelect.val());
	scaleSelect.change( function(event){
		scaleSelect.prop("zoomval", scaleSelect.val());
	});

});

refreshIntervall = window.setInterval("reloadImage(false)", 500);
