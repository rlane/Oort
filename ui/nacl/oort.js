oort = null;  // Global application object.

function handleMessage(message_event) {
	alert(message_event.data);
}

function moduleDidStartLoad() {
}

function moduleLoadProgress(event) {
	var last_slash = event.url.lastIndexOf("/");
	var filename = event.url.substring(last_slash+1);

	if (event.lengthComputable && event.total > 0) {
		var loadPercent = Math.round(event.loaded / event.total * 100.0);
		var loadPercentString = loadPercent + '%';
		var text = filename + ' ' + loadPercentString;
		$("#progress").html(text)
	}
}

function moduleLoadError() {
}

function moduleLoadAbort() {
}

function moduleDidLoad() {
	oort = document.getElementById('oort');
	oort.postMessage('start');
	oort.focus();
	$("#loading").fadeOut("slow")
}

function moduleDidEndLoad() {
	var lastError = event.target.lastError;
	if (lastError != undefined) {
		$("#progress").html(lastError)
	}
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
