CREATE DATABASE IF NOT EXISTS onlineoj CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE onlineoj;

CREATE TABLE users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(50) NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,
    role ENUM('user', 'admin') NOT NULL DEFAULT 'user',
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_username (username)
) ENGINE=InnoDB;

CREATE TABLE problems (
    id INT AUTO_INCREMENT PRIMARY KEY,
    title VARCHAR(200) NOT NULL,
    description TEXT NOT NULL,
    difficulty ENUM('easy', 'medium', 'hard') NOT NULL DEFAULT 'easy',
    code_template TEXT,
    time_limit_ms INT NOT NULL DEFAULT 2000,
    memory_limit_kb INT NOT NULL DEFAULT 262144,
    author_id INT NOT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (author_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_difficulty (difficulty),
    INDEX idx_author (author_id)
) ENGINE=InnoDB;

CREATE TABLE tags (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(50) NOT NULL UNIQUE
) ENGINE=InnoDB;

CREATE TABLE problem_tags (
    problem_id INT NOT NULL,
    tag_id INT NOT NULL,
    PRIMARY KEY (problem_id, tag_id),
    FOREIGN KEY (problem_id) REFERENCES problems(id) ON DELETE CASCADE,
    FOREIGN KEY (tag_id) REFERENCES tags(id) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE test_cases (
    id INT AUTO_INCREMENT PRIMARY KEY,
    problem_id INT NOT NULL,
    case_number INT NOT NULL,
    input_data MEDIUMTEXT NOT NULL,
    expected_output MEDIUMTEXT NOT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (problem_id) REFERENCES problems(id) ON DELETE CASCADE,
    INDEX idx_problem_case (problem_id, case_number)
) ENGINE=InnoDB;

CREATE TABLE submissions (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL,
    problem_id INT NOT NULL,
    code MEDIUMTEXT NOT NULL,
    status ENUM('pending', 'compiling', 'running', 'accepted',
                'wrong_answer', 'time_limit_exceeded',
                'memory_limit_exceeded', 'compilation_error',
                'system_error') NOT NULL DEFAULT 'pending',
    failed_case INT DEFAULT NULL,
    time_used_ms INT DEFAULT NULL,
    memory_used_kb INT DEFAULT NULL,
    passed_cases INT DEFAULT 0,
    total_cases INT DEFAULT 0,
    compile_error TEXT,
    detail_json JSON,
    submitted_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    judged_at DATETIME DEFAULT NULL,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (problem_id) REFERENCES problems(id) ON DELETE CASCADE,
    INDEX idx_user (user_id),
    INDEX idx_problem (problem_id),
    INDEX idx_status (status),
    INDEX idx_submitted (submitted_at)
) ENGINE=InnoDB;

-- Seed data: admin user (password: admin123, bcrypt hash)
INSERT INTO users (username, password_hash, role) VALUES
    ('admin', '$2b$12$LJ3m4ys3Lk0TSwHCpNqr4eYfKmV8k4aJwXQp0F9eZ3Q5a7b9c1d2E', 'admin');

-- Seed data: preset tags
INSERT INTO tags (name) VALUES
    ('动态规划'),
    ('贪心'),
    ('数学'),
    ('图论'),
    ('字符串'),
    ('数据结构'),
    ('搜索'),
    ('排序');
