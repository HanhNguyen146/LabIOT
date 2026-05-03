let ws = null;
let reconnectTimer = null;

let fan1Btn = null;
let fan2Btn = null;
let fan1Status = null;
let fan2Status = null;
let fan1Icon = null;
let fan2Icon = null;
let ledStatus = null;
let ledIcon = null;
let neoStatus = null;
let neoIcon = null;
let neoColor = null;

function applyDeviceUI(device, state, fault) {
    const isFan1 = device === 'fan1';
    const btn = isFan1 ? fan1Btn : fan2Btn;
    const statusEl = isFan1 ? fan1Status : fan2Status;
    const iconEl = isFan1 ? fan1Icon : fan2Icon;

    if (!btn || !statusEl || !iconEl) return;

    btn.checked = !!state;

    if (fault) {
        statusEl.textContent = 'Lỗi kết nối / lệch trạng thái';
        statusEl.className = 'mt-3 fw-semibold text-danger';
        iconEl.classList.remove('fa-spin');
        return;
    }

    statusEl.textContent = state ? 'Đang bật' : 'Đang tắt';
    statusEl.className = isFan1
        ? (state ? 'mt-3 fw-semibold text-success' : 'mt-3 fw-semibold text-muted')
        : (state ? 'mt-3 fw-semibold text-info' : 'mt-3 fw-semibold text-muted');

    if (state) {
        iconEl.classList.add('fa-spin');
    } else {
        iconEl.classList.remove('fa-spin');
    }
}

function sendDeviceState(device, state) {
    if (!ws || ws.readyState !== WebSocket.OPEN) {
        return;
    }

    ws.send(JSON.stringify({
        action: 'toggle_device',
        device,
        state: !!state
    }));
}

function setLedUI(mode) {
    if (!ledStatus || !ledIcon) return;

    if (mode === 'on') {
        ledStatus.textContent = 'Đang bật';
        ledStatus.className = 'mt-2 fw-semibold text-warning';
        ledIcon.classList.add('fa-beat');
    } else if (mode === 'off') {
        ledStatus.textContent = 'Đang tắt';
        ledStatus.className = 'mt-2 fw-semibold text-muted';
        ledIcon.classList.remove('fa-beat');
    } else {
        ledStatus.textContent = 'Auto';
        ledStatus.className = 'mt-2 fw-semibold text-secondary';
        ledIcon.classList.remove('fa-beat');
    }
}

function setNeoUI(mode, color) {
    if (!neoStatus || !neoIcon) return;

    if (mode === 'color') {
        neoStatus.textContent = color ? `Màu ${color.toUpperCase()}` : 'Đã đổi màu';
        neoStatus.className = 'mt-2 fw-semibold text-primary';
        neoIcon.classList.add('fa-beat');
    } else {
        neoStatus.textContent = 'Auto';
        neoStatus.className = 'mt-2 fw-semibold text-secondary';
        neoIcon.classList.remove('fa-beat');
    }
}

function sendLedCommand(cmd) {
    if (!ws || ws.readyState !== WebSocket.OPEN) {
        return;
    }

    ws.send(JSON.stringify({
        action: 'led_control',
        cmd
    }));
    setLedUI(cmd);
}

function sendNeoAuto() {
    if (!ws || ws.readyState !== WebSocket.OPEN) {
        return;
    }

    ws.send(JSON.stringify({
        action: 'neo_control',
        cmd: 'auto'
    }));
    setNeoUI('auto');
}

function sendNeoColor() {
    if (!ws || ws.readyState !== WebSocket.OPEN || !neoColor) {
        return;
    }

    const hex = neoColor.value || '#00ff64';
    const r = parseInt(hex.slice(1, 3), 16);
    const g = parseInt(hex.slice(3, 5), 16);
    const b = parseInt(hex.slice(5, 7), 16);

    ws.send(JSON.stringify({
        action: 'neo_control',
        cmd: 'color',
        r,
        g,
        b
    }));
    setNeoUI('color', hex);
}

function scheduleReconnect() {
    if (reconnectTimer) return;
    reconnectTimer = setTimeout(() => {
        reconnectTimer = null;
        connectWebSocket();
    }, 1500);
}

function connectWebSocket() {
    ws = new WebSocket(`ws://${location.host}/ws`);

    ws.onopen = function() {
        console.log('WebSocket connected');
    };

    ws.onmessage = function(event) {
        try {
            const msg = JSON.parse(event.data);

            if (msg.type === 'device_sync') {
                applyDeviceUI(msg.device, !!msg.actual, !!msg.fault);
            }

            if (msg.type === 'device_status') {
                applyDeviceUI(msg.device, !!msg.state, false);
            }

            if (msg.type === 'device_fault') {
                applyDeviceUI(msg.device, !!msg.actual, true);
            }

            if (msg.type === 'led_control_ack') {
                setLedUI(msg.cmd);
            }

            if (msg.type === 'neo_control_ack') {
                if (msg.cmd === 'color' && typeof msg.r !== 'undefined') {
                    const hex = `#${Number(msg.r).toString(16).padStart(2, '0')}${Number(msg.g).toString(16).padStart(2, '0')}${Number(msg.b).toString(16).padStart(2, '0')}`;
                    setNeoUI('color', hex);
                } else {
                    setNeoUI('auto');
                }
            }
        } catch (e) {
            console.warn('Invalid WS message', e);
        }
    };

    ws.onclose = function() {
        console.warn('WebSocket disconnected, reconnecting...');
        scheduleReconnect();
    };

    ws.onerror = function() {
        ws.close();
    };
}

window.addEventListener('DOMContentLoaded', function() {
    fan1Btn = document.getElementById('fan1-btn');
    fan2Btn = document.getElementById('fan2-btn');
    fan1Status = document.getElementById('fan1Status');
    fan2Status = document.getElementById('fan2Status');
    fan1Icon = document.getElementById('fan1Icon');
    fan2Icon = document.getElementById('fan2Icon');
    ledStatus = document.getElementById('ledStatus');
    ledIcon = document.getElementById('ledIcon');
    neoStatus = document.getElementById('neoStatus');
    neoIcon = document.getElementById('neoIcon');
    neoColor = document.getElementById('neo-color');

    if (fan1Btn) {
        fan1Btn.addEventListener('change', (e) => {
            sendDeviceState('fan1', e.target.checked);
        });
    }

    if (fan2Btn) {
        fan2Btn.addEventListener('change', (e) => {
            sendDeviceState('fan2', e.target.checked);
        });
    }

    const ledOnBtn = document.getElementById('led-on-btn');
    const ledOffBtn = document.getElementById('led-off-btn');
    const ledAutoBtn = document.getElementById('led-auto-btn');
    const neoColorBtn = document.getElementById('neo-color-btn');
    const neoAutoBtn = document.getElementById('neo-auto-btn');

    if (ledOnBtn) ledOnBtn.addEventListener('click', () => sendLedCommand('on'));
    if (ledOffBtn) ledOffBtn.addEventListener('click', () => sendLedCommand('off'));
    if (ledAutoBtn) ledAutoBtn.addEventListener('click', () => sendLedCommand('auto'));
    if (neoColorBtn) neoColorBtn.addEventListener('click', sendNeoColor);
    if (neoAutoBtn) neoAutoBtn.addEventListener('click', sendNeoAuto);

    connectWebSocket();
});
