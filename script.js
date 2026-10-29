let chart;
const $ = (id) => document.getElementById(id);
const fmt = (v, digits=1) => (v === null || v === undefined || !Number.isFinite(Number(v))) ? "--" : Number(v).toFixed(digits);
const setText = (id, value) => { $(id).textContent = value; };

function renderAlerts(alerts, receivedAt) {
  const root = $("alertsList");
  root.replaceChildren();
  if (!receivedAt) {
    root.innerHTML = '<div class="empty-state">No data received yet. Check the ESP32 Wi-Fi and backend URL.</div>';
    return;
  }
  (alerts || []).forEach((alert) => {
    const item = document.createElement("div");
    item.className = "alert-item";
    const mark = document.createElement("div");
    mark.className = "alert-mark";
    mark.textContent = "!";
    const content = document.createElement("div");
    const title = document.createElement("strong");
    title.textContent = alert;
    const detail = document.createElement("p");
    detail.textContent = "Rule-based prototype alert";
    content.append(title, detail);
    item.append(mark, content);
    root.append(item);
  });
}

function updateDashboard(d) {
  const online = Boolean(d && d.received_at);
  $("connectionText").textContent = online ? "ESP32 data received" : "Waiting for ESP32";
  $("deviceId").textContent = d?.device_id || "predict-kit-01";
  setText("temperature", fmt(d?.temperature_c));
  setText("current", fmt(d?.current_a, 2));
  setText("voltage", fmt(d?.voltage_v, 1));
  setText("vibration", fmt(d?.vibration, 2));
  setText("door", d?.door_open === true ? "OPEN" : d?.door_open === false ? "CLOSED" : "--");
  setText("healthScore", d?.health_score ?? "--");
  setText("healthLabel", online ? (d.status || "Monitoring") : "Waiting for data");
  setText("healthDescription", online ? "Current estimate from simple demonstration rules." : "Power on your ESP32 and send a reading to get started.");
  setText("alertCount", online ? (d.alerts || []).filter(a => a !== "No rule-based alerts").length : "0");
  setText("lastUpdated", online ? `Updated ${new Date(d.received_at).toLocaleTimeString()}` : "No data received");
  setText("statusPill", online ? d.status.toUpperCase() : "OFFLINE");
  $("scoreRing").style.background = online
    ? `conic-gradient(#6fe0b5 ${(Number(d.health_score)||0)*3.6}deg, #ffffff25 0deg)`
    : "conic-gradient(#ffffff25 0deg, #ffffff25 360deg)";
  renderAlerts(d?.alerts, d?.received_at);
}

async function refresh() {
  try {
    const response = await fetch("/api/latest", {cache: "no-store"});
    if (!response.ok) throw new Error("API response was not OK");
    updateDashboard(await response.json());
    await refreshHistory();
  } catch (err) {
    $("connectionText").textContent = "Backend not reachable";
  }
}

async function refreshHistory() {
  try {
    const response = await fetch("/api/history", {cache: "no-store"});
    const rows = await response.json();
    const valid = rows.filter(r => r.temperature_c !== null && r.temperature_c !== undefined);
    $("chartMessage").textContent = valid.length ? `${valid.length} temperature readings shown` : "Waiting for temperature readings…";
    const labels = valid.map(r => new Date(r.received_at).toLocaleTimeString());
    const values = valid.map(r => Number(r.temperature_c));
    const ctx = $("temperatureChart").getContext("2d");
    if (chart) chart.destroy();
    if (window.Chart) {
      chart = new Chart(ctx, {
        type: "line",
        data: { labels, datasets: [{label:"Temperature (°C)", data:values, borderColor:"#4778f5", backgroundColor:"rgba(71,120,245,.10)", fill:true, tension:.32, pointRadius:2, borderWidth:2}]},
        options: {responsive:true, maintainAspectRatio:true, plugins:{legend:{display:false}}, scales:{x:{ticks:{maxTicksLimit:6, color:"#8a95a5", font:{size:10}}, grid:{display:false}}, y:{ticks:{color:"#8a95a5", font:{size:10}}, grid:{color:"#edf1f6"}}}}
      });
    }
  } catch (_) {
    $("chartMessage").textContent = "Could not load history.";
  }
}
refresh();
setInterval(refresh, 2000);
