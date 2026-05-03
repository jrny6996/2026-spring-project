package main

import (
	"database/sql"
	"errors"
	"time"

	_ "github.com/mattn/go-sqlite3"
)

var db *sql.DB

type User struct {
	ID           int64
	Username     string
	PasswordHash string
}

func initDB(path string) error {
	d, err := sql.Open("sqlite3", path+"?_foreign_keys=on")
	if err != nil {
		return err
	}
	d.SetMaxOpenConns(1)
	if err := d.Ping(); err != nil {
		return err
	}
	if _, err := d.Exec(`
CREATE TABLE IF NOT EXISTS users (
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	username TEXT NOT NULL UNIQUE,
	password_hash TEXT NOT NULL,
	created_at TEXT NOT NULL
);`); err != nil {
		return err
	}
	if _, err := d.Exec(`
CREATE TABLE IF NOT EXISTS wins (
	user_id INTEGER NOT NULL,
	night INTEGER NOT NULL CHECK (night >= 1 AND night <= 7),
	won_at TEXT NOT NULL,
	PRIMARY KEY (user_id, night),
	FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);`); err != nil {
		return err
	}
	// Legacy: copy night_completions into wins if that table exists (older installs).
	_, _ = d.Exec(`
INSERT OR IGNORE INTO wins (user_id, night, won_at)
SELECT user_id, night, completed_at FROM night_completions`)

	db = d
	return nil
}

func recordWin(userID int64, night int) error {
	if userID <= 0 {
		return nil
	}
	if night < 1 {
		night = 1
	}
	if night > 7 {
		night = 7
	}
	_, err := db.Exec(`
INSERT INTO wins (user_id, night, won_at) VALUES (?, ?, ?)
ON CONFLICT(user_id, night) DO UPDATE SET won_at = excluded.won_at`,
		userID, night, time.Now().UTC().Format(time.RFC3339))
	return err
}

func hasUserWonNight(userID int64, night int) (bool, error) {
	if userID <= 0 || night < 1 || night > 7 {
		return false, nil
	}
	var one int
	err := db.QueryRow(
		`SELECT 1 FROM wins WHERE user_id = ? AND night = ? LIMIT 1`,
		userID, night,
	).Scan(&one)
	if errors.Is(err, sql.ErrNoRows) {
		return false, nil
	}
	if err != nil {
		return false, err
	}
	return true, nil
}

func listCompletedNights(userID int64) ([]int, error) {
	if userID <= 0 {
		return nil, nil
	}
	rows, err := db.Query(
		`SELECT night FROM wins WHERE user_id = ? ORDER BY night ASC`,
		userID,
	)
	if err != nil {
		return nil, err
	}
	defer rows.Close()
	var out []int
	for rows.Next() {
		var n int
		if err := rows.Scan(&n); err != nil {
			return nil, err
		}
		out = append(out, n)
	}
	return out, rows.Err()
}

func getUserByUsername(username string) (*User, error) {
	row := db.QueryRow(`SELECT id, username, password_hash FROM users WHERE username = ?`, username)
	var u User
	if err := row.Scan(&u.ID, &u.Username, &u.PasswordHash); err != nil {
		return nil, err
	}
	return &u, nil
}

func createUser(username, passwordHash string) (int64, error) {
	res, err := db.Exec(
		`INSERT INTO users (username, password_hash, created_at) VALUES (?, ?, ?)`,
		username, passwordHash, time.Now().UTC().Format(time.RFC3339),
	)
	if err != nil {
		return 0, err
	}
	return res.LastInsertId()
}
