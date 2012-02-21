oort = null;  // Global application object.

// Indicate success when the NaCl module has loaded.
function moduleDidLoad() {
	oort = document.getElementById('oort');
	oort.postMessage('start');
	oort.focus();
}

// Handle a message coming from the NaCl module.
function handleMessage(message_event) {
	alert(message_event.data);
}

$(document).ready(function(){
	$("#menu-return").click(function(event){
		$("#overlay").hide();
		$("#show-overlay").show();
		oort.focus();
	});

	$("#show-overlay").click(function(event){
		$("#overlay").show();
		$("#show-overlay").hide();
		oort.focus();
	});

	$("#menu-newgame").click(function(event){
		oort.postMessage('start');
		oort.focus();
	});
});
