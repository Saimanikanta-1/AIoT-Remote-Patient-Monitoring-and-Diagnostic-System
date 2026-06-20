"""
SQLite database module for Patient Remote Monitoring System.
"""

import sqlite3
import os
from datetime import datetime
from contextlib import contextmanager

DB_PATH = os.path.join(os.path.dirname(__file__), "patient_monitoring.db")


def get_connection():
    """Return a SQLite connection with row factory enabled."""
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn


@contextmanager
def db_session():
    """Context manager for database transactions."""
    conn = get_connection()
    try:
        yield conn
        conn.commit()
    except Exception:
        conn.rollback()
        raise
    finally:
        conn.close()


def init_db():
    """Create patient_data table if it does not exist."""
    with db_session() as conn:
        conn.execute("""
            CREATE TABLE IF NOT EXISTS patient_data (
                id          INTEGER PRIMARY KEY AUTOINCREMENT,
                timestamp   TEXT    NOT NULL,
                patient_id  TEXT    DEFAULT 'PAT-001',
                temperature REAL    NOT NULL,
                humidity    REAL    NOT NULL,
                heart_rate  INTEGER NOT NULL,
                spo2        INTEGER NOT NULL,
                risk_level  TEXT    NOT NULL,
                z_temp      REAL    DEFAULT 0,
                z_humidity  REAL    DEFAULT 0,
                z_heart_rate REAL   DEFAULT 0,
                z_spo2      REAL    DEFAULT 0,
                anomaly_status TEXT DEFAULT 'OK'
            )
        """)
        conn.execute("""
            CREATE INDEX IF NOT EXISTS idx_patient_data_timestamp
            ON patient_data (timestamp DESC)
        """)


def insert_patient_data(data: dict) -> int:
    """Insert a new patient reading and return the row id."""
    timestamp = data.get("timestamp") or datetime.utcnow().isoformat() + "Z"

    with db_session() as conn:
        cursor = conn.execute(
            """
            INSERT INTO patient_data
                (timestamp, patient_id, temperature, humidity,
                 heart_rate, spo2, risk_level,
                 z_temp, z_humidity, z_heart_rate, z_spo2, anomaly_status)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """,
            (
                timestamp,
                data.get("patient_id", "PAT-001"),
                float(data["temperature"]),
                float(data["humidity"]),
                int(data["heart_rate"]),
                int(data["spo2"]),
                data.get("risk_level", "Normal"),
                float(data.get("z_temp", 0)),
                float(data.get("z_humidity", 0)),
                float(data.get("z_heart_rate", 0)),
                float(data.get("z_spo2", 0)),
                data.get("anomaly_status", "OK"),
            ),
        )
        return cursor.lastrowid


def get_latest_reading(limit: int = 1):
    """Return the most recent patient reading(s)."""
    with db_session() as conn:
        rows = conn.execute(
            """
            SELECT * FROM patient_data
            ORDER BY id DESC
            LIMIT ?
            """,
            (limit,),
        ).fetchall()
        return [dict(row) for row in rows]


def get_readings_history(limit: int = 50):
    """Return recent readings for dashboard charts (oldest first)."""
    with db_session() as conn:
        rows = conn.execute(
            """
            SELECT * FROM (
                SELECT * FROM patient_data
                ORDER BY id DESC
                LIMIT ?
            ) sub
            ORDER BY id ASC
            """,
            (limit,),
        ).fetchall()
        return [dict(row) for row in rows]


def get_stats():
    """Return aggregate statistics for dashboard summary cards."""
    with db_session() as conn:
        row = conn.execute("""
            SELECT
                COUNT(*)                          AS total_readings,
                AVG(temperature)                  AS avg_temperature,
                AVG(humidity)                     AS avg_humidity,
                AVG(heart_rate)                   AS avg_heart_rate,
                AVG(spo2)                         AS avg_spo2,
                SUM(CASE WHEN risk_level='Critical' THEN 1 ELSE 0 END) AS critical_count,
                SUM(CASE WHEN risk_level='Warning'  THEN 1 ELSE 0 END) AS warning_count
            FROM patient_data
        """).fetchone()
        return dict(row) if row else {}
