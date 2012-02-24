oort = null;  // Global application object.

var last_progress_event = null;
var downloadedBytes = 0;
var totalBytes = 31742396;

function handleMessage(message_event) {
	alert(message_event.data);
}

function postMessage(msg) {
	oort.postMessage(JSON.stringify(msg, null, 2));
}

function moduleDidStartLoad() {
}

function updateProgressbar(filename, totalFraction) {
	var loadPercent = Math.round(totalFraction * 100.0);
	$("#progress-filename").html(filename)
	$("#progress-bar").css("width", loadPercent+"%")
}

function moduleLoadProgress(event) {
	var last_slash = event.url.lastIndexOf("/");
	var filename = event.url.substring(last_slash+1);

	if (last_progress_event == null) {
		downloadedBytes += event.loaded;
	} else if (event.url != last_progress_event.url) {
		downloadedBytes += (last_progress_event.total - last_progress_event.loaded);
		downloadedBytes += event.loaded;
	} else {
		downloadedBytes += (event.loaded - last_progress_event.loaded);
	}

	last_progress_event = event;

	updateProgressbar(filename, downloadedBytes/totalBytes);

	//console.log(filename + " / " + event.loaded + " / " + event.total);
	//console.log("downloaded: " + downloadedBytes);
}

function moduleLoadError() {
}

function moduleLoadAbort() {
}

function moduleDidLoad() {
	oort = document.getElementById('oort');
	postMessage({ key: "start", scenario: "scenarios/demo1.json" });
	oort.focus();
	updateProgressbar("", 1);
	$("#loading").fadeOut("slow")
	$("#overlay").fadeIn("slow")
}

function moduleDidEndLoad() {
	var lastError = event.target.lastError;
	if (lastError != undefined) {
		$("#progress-filename").html(lastError)
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
		postMessage({ key: "start", scenario: "scenarios/basic.json" });
		oort.focus();
	});
});
