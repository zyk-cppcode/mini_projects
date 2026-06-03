function renderSidebar() {
    const sidebar = document.getElementById('sidebar');
    if (!sidebar) return;
    const user = getCurrentUser();
    const admin = isAdmin();
    const path = window.location.pathname;

    let html = '<div class="sidebar-logo"><span class="icon">◈</span>OnlineOJ</div>';
    html += '<div class="sidebar-nav">';

    const links = [
        { href: '/', icon: '📋', label: '题目列表' },
        { href: '/leaderboard.html', icon: '🏆', label: '排行榜' },
    ];
    if (user) {
        links.push({ href: '/submissions.html', icon: '📜', label: '提交记录' });
        links.push({ href: '/statistics.html', icon: '📊', label: '个人统计' });
        if (admin) {
            links.push({ href: '/admin/problems.html', icon: '⚙', label: '管理后台' });
        }
    }

    links.forEach(l => {
        const active = (l.href === '/' && (path === '/' || path === '/index.html'))
            || (l.href !== '/' && path.endsWith(l.href));
        html += `<a href="${l.href}" class="${active ? 'active' : ''}">
            <span class="nav-icon">${l.icon}</span><span>${l.label}</span></a>`;
    });

    html += '</div>';

    html += '<div class="sidebar-user">';
    if (user) {
        html += `<span class="username">${htmlEscape(user.username)}</span>`;
        html += `<a class="logout-btn" href="#" onclick="doLogout(event)">登出</a>`;
    } else {
        html += `<a href="/login.html">登录</a>`;
    }
    html += '</div>';

    sidebar.innerHTML = html;
}

function showToast(msg, type = 'info') {
    const el = document.getElementById('toast');
    if (!el) return;
    el.textContent = msg;
    el.className = 'toast ' + type;
    el.style.display = 'block';
    setTimeout(() => el.style.display = 'none', 3000);
}

function renderPagination(page, total, pageSize, onPage) {
    const totalPages = Math.ceil(total / pageSize) || 1;
    let html = '<div class="pagination">';
    for (let i = 1; i <= totalPages; i++) {
        if (i === page) html += `<strong>${i}</strong>`;
        else html += `<a href="#" onclick="event.preventDefault();${onPage}(${i})">${i}</a>`;
    }
    html += '</div>';
    return html;
}

function doLogout(e) {
    e.preventDefault();
    logout().then(() => { window.location.href = '/login.html'; });
}
