const BASE = '';

async function request(method, path, body) {
    const opts = { method, headers: { 'Content-Type': 'application/json' }, credentials: 'same-origin' };
    if (body) opts.body = JSON.stringify(body);
    const resp = await fetch(BASE + path, opts);
    const data = await resp.json();
    if (!resp.ok) {
        const err = new Error(data.error || '请求失败');
        err.code = data.code || 'ERROR';
        err.status = resp.status;
        throw err;
    }
    return data.data;
}

const api = {
    get: (path) => request('GET', path),
    post: (path, body) => request('POST', path, body),
    put: (path, body) => request('PUT', path, body),
    del: (path) => request('DELETE', path),
};
