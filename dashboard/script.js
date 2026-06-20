/**
 * Patient Remote Monitoring Dashboard
 * Real-time vitals display with Chart.js
 */

const API_BASE = "http://localhost:5001/api";
const POLL_INTERVAL_MS = 3000;
const MAX_CHART_POINTS = 30;

// Chart instances
let tempHumChart = null;
let vitalsChart = null;

// Chart configuration helper
function chartDefaults() {
  return {
    responsive: true,
    maintainAspectRatio: false,
    animation: { duration: 400 },
    scales: {
      x: {
        ticks: { color: "#94a3b8", maxTicksLimit: 8 },
        grid: { color: "rgba(51,65,85,0.4)" },
      },
      y: {
        ticks: { color: "#94a3b8" },
        grid: { color: "rgba(51,65,85,0.4)" },
      },
    },
    plugins: {
      legend: {
        labels: { color: "#f1f5f9" },
      },
    },
  };
}

function initCharts() {
  const tempHumCtx = document.getElementById("tempHumChart").getContext("2d");
  tempHumChart = new Chart(tempHumCtx, {
    type: "line",
    data: {
      labels: [],
      datasets: [
        {
          label: "Temperature (°C)",
          data: [],
          borderColor: "#ef4444",
          backgroundColor: "rgba(239,68,68,0.1)",
          tension: 0.4,
          fill: true,
        },
        {
          label: "Humidity (%)",
          data: [],
          borderColor: "#3b82f6",
          backgroundColor: "rgba(59,130,246,0.1)",
          tension: 0.4,
          fill: true,
        },
      ],
    },
    options: chartDefaults(),
  });

  const vitalsCtx = document.getElementById("vitalsChart").getContext("2d");
  vitalsChart = new Chart(vitalsCtx, {
    type: "line",
    data: {
      labels: [],
      datasets: [
        {
          label: "Heart Rate (bpm)",
          data: [],
          borderColor: "#a855f7",
          backgroundColor: "rgba(168,85,247,0.1)",
          tension: 0.4,
          fill: true,
        },
        {
          label: "SpO2 (%)",
          data: [],
          borderColor: "#22c55e",
          backgroundColor: "rgba(34,197,94,0.1)",
          tension: 0.4,
          fill: true,
        },
      ],
    },
    options: chartDefaults(),
  });
}

function formatTime(timestamp) {
  if (!timestamp) return "--";
  const d = new Date(timestamp.endsWith("Z") ? timestamp : timestamp + "Z");
  return d.toLocaleTimeString();
}

function formatTimestamp(timestamp) {
  if (!timestamp) return "--";
  const d = new Date(timestamp.endsWith("Z") ? timestamp : timestamp + "Z");
  return d.toLocaleString();
}

function updateConnectionStatus(online) {
  const el = document.getElementById("connectionStatus");
  el.textContent = online ? "Online" : "Offline";
  el.className = `badge ${online ? "badge-online" : "badge-offline"}`;
}

function updateVitalCards(data) {
  document.getElementById("liveTemp").textContent = data.temperature?.toFixed(1) ?? "--";
  document.getElementById("liveHumidity").textContent = data.humidity?.toFixed(1) ?? "--";
  document.getElementById("liveHR").textContent = data.heart_rate ?? "--";
  document.getElementById("liveSpO2").textContent = data.spo2 ?? "--";

  document.getElementById("zTemp").textContent = `Z: ${(data.z_temp ?? 0).toFixed(2)}`;
  document.getElementById("zHumidity").textContent = `Z: ${(data.z_humidity ?? 0).toFixed(2)}`;
  document.getElementById("zHR").textContent = `Z: ${(data.z_heart_rate ?? 0).toFixed(2)}`;
  document.getElementById("zSpO2").textContent = `Z: ${(data.z_spo2 ?? 0).toFixed(2)}`;
}

function updateAlertBanner(data) {
  const banner = document.getElementById("alertBanner");
  const riskEl = document.getElementById("riskStatus");
  const alertEl = document.getElementById("alertStatus");
  const risk = data.risk_level || "Normal";
  const anomaly = data.anomaly_status || "OK";

  banner.className = "alert-banner";
  if (risk === "Critical") {
    banner.classList.add("critical");
    riskEl.textContent = "CRITICAL";
    alertEl.textContent = anomaly === "CRITICAL_ANOMALY"
      ? "Critical anomaly detected by Edge AI — immediate attention required"
      : "Critical health risk detected — alert triggered on device";
  } else if (risk === "Warning") {
    banner.classList.add("warning");
    riskEl.textContent = "WARNING";
    alertEl.textContent = "Vitals approaching abnormal range — monitor closely";
  } else {
    banner.classList.add("normal");
    riskEl.textContent = "NORMAL";
    alertEl.textContent = "All vitals within normal range";
  }
}

function updateCharts(history) {
  const labels = history.map((r) => formatTime(r.timestamp));
  const temps = history.map((r) => r.temperature);
  const hums = history.map((r) => r.humidity);
  const hrs = history.map((r) => r.heart_rate);
  const spo2s = history.map((r) => r.spo2);

  tempHumChart.data.labels = labels;
  tempHumChart.data.datasets[0].data = temps;
  tempHumChart.data.datasets[1].data = hums;
  tempHumChart.update("none");

  vitalsChart.data.labels = labels;
  vitalsChart.data.datasets[0].data = hrs;
  vitalsChart.data.datasets[1].data = spo2s;
  vitalsChart.update("none");
}

function updateStats(stats) {
  if (!stats || !stats.total_readings) return;
  document.getElementById("totalReadings").textContent = stats.total_readings ?? 0;
  document.getElementById("avgTemp").textContent =
    stats.avg_temperature ? `${stats.avg_temperature.toFixed(1)}°C` : "--";
  document.getElementById("avgHR").textContent =
    stats.avg_heart_rate ? `${Math.round(stats.avg_heart_rate)} bpm` : "--";
  document.getElementById("warningCount").textContent = stats.warning_count ?? 0;
  document.getElementById("criticalCount").textContent = stats.critical_count ?? 0;
}

function updateTable(history) {
  const tbody = document.getElementById("readingsBody");
  if (!history.length) {
    tbody.innerHTML = '<tr><td colspan="7" class="empty-row">No data received yet</td></tr>';
    return;
  }

  const rows = [...history].reverse().slice(0, 15);
  tbody.innerHTML = rows
    .map(
      (r) => `
    <tr>
      <td>${formatTimestamp(r.timestamp)}</td>
      <td>${r.temperature?.toFixed(1)}</td>
      <td>${r.humidity?.toFixed(1)}</td>
      <td>${r.heart_rate}</td>
      <td>${r.spo2}</td>
      <td class="risk-${(r.risk_level || "normal").toLowerCase()}">${r.risk_level}</td>
      <td>${r.anomaly_status || "OK"}</td>
    </tr>`
    )
    .join("");
}

async function fetchJSON(url) {
  const response = await fetch(url);
  if (!response.ok) throw new Error(`HTTP ${response.status}`);
  return response.json();
}

async function refreshDashboard() {
  try {
    const [latestRes, historyRes, statsRes] = await Promise.all([
      fetchJSON(`${API_BASE}/patient-data/latest`),
      fetchJSON(`${API_BASE}/patient-data/history?limit=${MAX_CHART_POINTS}`),
      fetchJSON(`${API_BASE}/patient-data/stats`),
    ]);

    updateConnectionStatus(true);

    if (latestRes.data) {
      updateVitalCards(latestRes.data);
      updateAlertBanner(latestRes.data);
      document.getElementById("lastUpdate").textContent =
        `Last update: ${formatTimestamp(latestRes.data.timestamp)}`;
    }

    if (historyRes.data?.length) {
      updateCharts(historyRes.data);
      updateTable(historyRes.data);
    }

    if (statsRes.stats) {
      updateStats(statsRes.stats);
    }
  } catch (err) {
    console.warn("Dashboard fetch error:", err.message);
    updateConnectionStatus(false);
    document.getElementById("lastUpdate").textContent = "Backend unreachable — start Flask API";
  }
}

// Initialize
document.addEventListener("DOMContentLoaded", () => {
  initCharts();
  refreshDashboard();
  setInterval(refreshDashboard, POLL_INTERVAL_MS);
});
