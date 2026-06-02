function renderNavbar() {
    const nav = document.getElementById('navbar');
    if (!nav) return;
    const user = getCurrentUser();
    const admin = isAdmin();
    let html = `<ul><li><strong><a href="/" class="contrast">OnlineOJ</a></strong></li></ul><ul>`;
    html += `<li><a href="/">题目</a></li>`;
    html += `<li><a href="/leaderboard.html">排行榜</a></li>`;
    if (user) {
        html += `<li><a href="/submissions.html">提交</a></li>`;
        html += `<li><a href="/statistics.html">统计</a></li>`;
        if (admin) {
            html += `<li><a href="/admin/problems.html">管理</a></li>`;
        }
        html += `<li><span>${htmlEscape(user.username)}</span></li>`;
        html += `<li><a href="#" onclick="doLogout()">登出</a></li>`;
    } else {
        html += `<li><a href="/login.html">登录</a></li>`;
        html += `<li><a href="/register.html">注册</a></li>`;
    }
    html += `</ul>`;
    nav.innerHTML = html;
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
        if (i === page) html += `<strong>${i}</strong> `;
        else html += `<a href="#" onclick="event.preventDefault();${onPage}(${i})">${i}</a> `;
    }
    html += `</div>`;
    return html;
}

async function doLogout() {
    await logout();
    window.location.href = '/';
}
