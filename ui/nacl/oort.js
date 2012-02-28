oort = null;  // Global application object.

var last_progress_event = null;
var downloadedBytes = 0;
var totalBytes = 31742396;

function handleMessage(message_event) {
	console.log(message_event.data);
}

function postMessage(msg) {
	console.log(["posted", msg]);
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
	postMessage({ key: "start", scenario: "scenarios/demo1.json", ais: [] });
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

function showMenu() {
	$("#overlay").show();
	$("#show-overlay").hide();
	oort.focus();
}

function hideMenu() {
	$("#overlay").fadeOut("slow");
	$("#show-overlay").show();
	oort.focus();
}

$(document).ready(function(){
	$("#menu-return").click(function(event){
		hideMenu();
	});

	$("#show-overlay").click(function(event){
		showMenu();
	});

	$("#menu-newgame").click(function(event){
		$("#newgame").show();
	});

	$("#newgame-btn").click(function(event){
		var ai01 = $("#ai01")[0].value;
		var ai02 = $("#ai02")[0].value;
		var ais = [{ filename: ai01 }, { filename: ai02 }];
		postMessage({ key: "start", scenario: "scenarios/basic.json", ais: ais });
		hideMenu();
		oort.focus();
		return false;
	});
});
