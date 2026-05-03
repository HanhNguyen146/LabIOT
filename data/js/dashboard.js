// Kết nối WebSocket tới server
const ws = new WebSocket(`ws://${location.host}/ws`);

ws.onopen = function() {
	console.log('WebSocket connected');
};

ws.onmessage = function(event) {
	try {
		const msg = JSON.parse(event.data);
		if (msg.type === 'sensor') {
			if (typeof msg.temperature !== 'undefined') {
				document.getElementById('temperature-value').textContent = msg.temperature.toFixed(1);
			}
			if (typeof msg.humidity !== 'undefined') {
				document.getElementById('humidity-value').textContent = msg.humidity.toFixed(1);
			}
			if (typeof msg.inferenceScore !== 'undefined') {
				document.getElementById('inference-score-value').textContent = msg.inferenceScore.toFixed(3);
			}
		}
	} catch (e) {
		// Không phải JSON hợp lệ hoặc không phải dữ liệu cảm biến
	}
};

ws.onclose = function() {
	console.log('WebSocket disconnected');
};
