function htmlEscape(str) {
    const div = document.createElement('div');
    div.textContent = str;
    return div.innerHTML;
}

function formatDate(d) {
    if (!d) return '-';
    return new Date(d).toLocaleString('zh-CN');
}

function statusLabel(status) {
    const map = {
        pending: '等待中', compiling: '编译中', running: '运行中',
        accepted: '通过', wrong_answer: '答案错误',
        time_limit_exceeded: '超时', memory_limit_exceeded: '超内存',
        compilation_error: '编译错误', system_error: '系统错误'
    };
    return map[status] || status;
}

function statusColor(status) {
    const map = {
        accepted: '#4caf50', wrong_answer: '#f44336',
        time_limit_exceeded: '#ff9800', memory_limit_exceeded: '#ff9800',
        compilation_error: '#2196f3', system_error: '#9e9e9e',
        pending: '#9e9e9e', compiling: '#2196f3', running: '#2196f3'
    };
    return map[status] || '#9e9e9e';
}

function getParam(name) {
    const p = new URLSearchParams(window.location.search);
    return p.get(name);
}
