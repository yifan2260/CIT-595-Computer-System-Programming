var myJSON = {"usage": $('#usage').html(), "max": $('#max').html(), "average": $('#average').html()};
var myObj = JSON.parse(myJSON);
var flag = false;// whether we have clicked the button or not
var myVar = setInterval(handle, 1000);
var thresholds = 70;

function handle() {
	if(flag){
		$.getJSON("/data",
			function (data) {
				if(data.usage >= thresholds) alert("Usage exceeded!");
				$('#usage').html("usage: " + String(data.usage));
				$('#max').html("max: " + String(data.max));
				$('#average').html("average: " + String(data.average));
			}	
		);
	}
}

function click(){
	flag = true;
}