let currentUser = null;

async function checkAuth() {
    try {
        currentUser = await api.get('/api/session');
    } catch {
        currentUser = null;
    }
    return currentUser;
}

async function login(username, password) {
    const data = await api.post('/api/login', { username, password });
    await checkAuth();
    return data;
}

async function register(username, password) {
    const data = await api.post('/api/register', { username, password });
    return data;
}

async function logout() {
    await api.post('/api/logout');
    currentUser = null;
}

function getCurrentUser() {
    return currentUser;
}

function isAdmin() {
    return currentUser && currentUser.role === 'admin';
}

function isLoggedIn() {
    return currentUser !== null;
}
