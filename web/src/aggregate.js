/** 将 webmodel.json 的分区上报汇总为大屏视图模型。 */

export const DATA_URL = '/data/dashboard.json';
export const PRED_URL = '/data/prediction.json';
export const REFRESH_MS = 60 * 60 * 1000;
const HISTORY_KEY = 'ncs-dashboard-history';
const HISTORY_LIMIT = 24 * 40;

const STATUS_KEYS = ['online', 'offline', 'broken'];

/** 典型充电时段权重（0-23 时），总和为 1。 */
export const HOUR_WEIGHTS = [
  0.012, 0.008, 0.006, 0.005, 0.007, 0.012,
  0.028, 0.048, 0.062, 0.052, 0.040, 0.046,
  0.058, 0.048, 0.040, 0.038, 0.046, 0.072,
  0.088, 0.076, 0.062, 0.046, 0.030, 0.020,
];

/** 周日到周六相对强度。 */
export const WEEKDAY_FACTOR = [0.78, 1.02, 1.04, 1.03, 1.06, 1.14, 0.86];

const WEEKDAYS = ['周日', '周一', '周二', '周三', '周四', '周五', '周六'];

export function parseStamp(text) {
  if (!text) return null;
  const normalized = String(text).trim().replace('T', ' ');
  const date = new Date(normalized.replace(/-/g, '/'));
  return Number.isNaN(date.getTime()) ? null : date;
}

export function pad(value) {
  return String(value).padStart(2, '0');
}

export function formatDateTime(date) {
  if (!(date instanceof Date) || Number.isNaN(date.getTime())) return '--';
  return `${date.getFullYear()}-${pad(date.getMonth() + 1)}-${pad(date.getDate())} ${pad(date.getHours())}:${pad(date.getMinutes())}:${pad(date.getSeconds())}`;
}

export function formatClock(date) {
  const weeks = '日一二三四五六';
  return `${formatDateTime(date)} 星期${weeks[date.getDay()]}`;
}

export function formatMoney(value) {
  return Number(value || 0).toLocaleString('zh-CN', {
    minimumFractionDigits: 2,
    maximumFractionDigits: 2,
  });
}

export function formatInt(value) {
  return Number(value || 0).toLocaleString('zh-CN');
}

function num(value) {
  const n = Number(value);
  return Number.isFinite(n) ? n : 0;
}

function regionLabel(row) {
  const city = String(row.City || '').replace(/市$/, '');
  const district = String(row.District || '');
  if (district && city && !district.includes(city)) return `${city}·${district}`;
  return district || city || String(row.Province || '未知区域');
}

function statusBag() {
  return { online: 0, offline: 0, broken: 0 };
}

function addStatus(bag, list) {
  for (const item of list || []) {
    const key = String(item.Status || '').toLowerCase();
    if (STATUS_KEYS.includes(key)) bag[key] += num(item.ChargerNum);
  }
}

function addTypes(bucket, list) {
  let found = false;
  for (const item of list || []) {
    found = true;
    const key = String(item.Type || item.Status || '').toLowerCase();
    const count = num(item.ChargerNum);
    if (key.includes('fast') || key.includes('快')) bucket.fast += count;
    else bucket.slow += count;
  }
  return found;
}

function loadHistory() {
  try {
    const raw = JSON.parse(localStorage.getItem(HISTORY_KEY) || '[]');
    return Array.isArray(raw) ? raw : [];
  } catch {
    return [];
  }
}

function saveHistory(list) {
  try {
    localStorage.setItem(HISTORY_KEY, JSON.stringify(list.slice(-HISTORY_LIMIT)));
  } catch {
    /* ignore quota */
  }
}

function snapshotOf(view) {
  return {
    datetime: view.datetime,
    orders: view.metrics.orders,
    revenue: view.metrics.revenue,
    users: view.metrics.users,
  };
}

export function rememberSnapshot(view) {
  if (!view.datetime) return loadHistory();
  const history = loadHistory();
  const next = snapshotOf(view);
  const index = history.findIndex((item) => item.datetime === next.datetime);
  if (index >= 0) history[index] = next;
  else history.push(next);
  history.sort((a, b) => String(a.datetime).localeCompare(String(b.datetime)));
  saveHistory(history);
  return history;
}

function dayKey(date) {
  return `${date.getFullYear()}-${pad(date.getMonth() + 1)}-${pad(date.getDate())}`;
}

function scaleByWeekday(base, date, anchor) {
  const factor = WEEKDAY_FACTOR[date.getDay()] / WEEKDAY_FACTOR[anchor.getDay() || 0];
  return Math.max(0, Math.round(base * factor * 100) / 100);
}

function buildTrend(view, history) {
  const anchor = parseStamp(view.datetime) || new Date();
  const byDay = {};
  for (const item of history) {
    const stamp = parseStamp(item.datetime);
    if (!stamp) continue;
    byDay[dayKey(stamp)] = num(item.revenue);
  }
  const points = [];
  let realDays = 0;
  for (let i = 29; i >= 0; i -= 1) {
    const date = new Date(anchor.getTime());
    date.setHours(0, 0, 0, 0);
    date.setDate(date.getDate() - i);
    const key = dayKey(date);
    const real = Object.prototype.hasOwnProperty.call(byDay, key);
    if (real) realDays += 1;
    points.push({
      day: `${pad(date.getMonth() + 1)}-${pad(date.getDate())}`,
      revenue: real ? byDay[key] : scaleByWeekday(view.metrics.revenue, date, anchor),
      real,
    });
  }
  return { points, estimated: realDays < 7 };
}

function buildHeatmap(view, history) {
  const real = {};
  const sorted = [...history].sort((a, b) => String(a.datetime).localeCompare(String(b.datetime)));
  for (let i = 1; i < sorted.length; i += 1) {
    const prev = parseStamp(sorted[i - 1].datetime);
    const cur = parseStamp(sorted[i].datetime);
    if (!prev || !cur) continue;
    const hours = (cur.getTime() - prev.getTime()) / 3600000;
    if (hours <= 0 || hours > 3) continue;
    const dropped = (num(sorted[i - 1].orders) / 24) * hours;
    const added = num(sorted[i].orders) - num(sorted[i - 1].orders) + dropped;
    const key = `${cur.getDay()}-${cur.getHours()}`;
    real[key] = Math.max(0, added);
  }

  const cells = [];
  let max = 0;
  let realCells = 0;
  for (let weekday = 0; weekday < 7; weekday += 1) {
    for (let hour = 0; hour < 24; hour += 1) {
      const key = `${weekday}-${hour}`;
      const estimated = view.metrics.orders * HOUR_WEIGHTS[hour] * WEEKDAY_FACTOR[weekday];
      const value = Object.prototype.hasOwnProperty.call(real, key) ? real[key] : estimated;
      if (Object.prototype.hasOwnProperty.call(real, key)) realCells += 1;
      max = Math.max(max, value);
      cells.push([hour, weekday, Math.round(value * 10) / 10]);
    }
  }
  return {
    hours: Array.from({ length: 24 }, (_, hour) => `${pad(hour)}:00`),
    weekdays: WEEKDAYS,
    cells,
    max: max || 1,
    estimated: realCells < 12,
  };
}

function hourLabel(text) {
  const stamp = parseStamp(text);
  if (stamp) return `${pad(stamp.getHours())}:00`;
  const raw = String(text || '');
  return raw.length >= 16 ? raw.slice(11, 16) : raw;
}

function buildPredictionFallback(view) {
  const start = parseStamp(view.datetime) || new Date();
  const raw = [];
  for (let offset = 1; offset <= 24; offset += 1) {
    const time = new Date(start.getTime() + offset * 3600000);
    const weight = HOUR_WEIGHTS[time.getHours()] * WEEKDAY_FACTOR[time.getDay()];
    raw.push({ time, weight, peak: [8, 9, 10, 17, 18, 19, 20].includes(time.getHours()) });
  }
  const weightSum = raw.reduce((sum, item) => sum + item.weight, 0) || 1;
  return {
    source: 'fallback',
    message: '由近 24h 订单按峰谷外推，高峰标红',
    metrics: null,
    history: [],
    points: raw.map((item) => ({
      label: `${pad(item.time.getHours())}:00`,
      load: Math.round((view.metrics.orders * item.weight) / weightSum * 10) / 10,
      peak: item.peak,
      idle: 0,
    })),
  };
}

export function buildPredictionFromMl(payload) {
  const series = Array.isArray(payload?.series) ? payload.series : [];
  if (!payload || payload.status === 'empty' || series.length === 0) {
    return {
      source: 'empty',
      message: payload?.message || '暂无预测数据，请写入 hourly_load.csv 后运行 ml/predict.py',
      metrics: payload?.metrics || null,
      history: [],
      points: [],
    };
  }
  return {
    source: 'ml',
    message: payload.message || '模型输出，高峰标红',
    metrics: payload.metrics || null,
    generatedAt: payload.generated_at || '',
    history: (payload.history || []).map((item) => ({
      label: hourLabel(item.target_time),
      load: num(item.actual_energy),
    })),
    points: series.map((item) => ({
      label: hourLabel(item.target_time),
      load: num(item.predicted_energy),
      peak: Boolean(Number(item.is_peak)),
      idle: num(item.predicted_idle),
    })),
  };
}

export function aggregate(report, predictionPayload) {
  const rows = Array.isArray(report?.data) ? report.data : [];
  const status = statusBag();
  const types = { fast: 0, slow: 0 };
  let hasType = false;
  const ranking = [];
  let orders = 0;
  let revenue = 0;
  let users = 0;

  for (const row of rows) {
    orders += num(row['24hChargedOrder']);
    revenue += num(row['24hTotalIncome']);
    users += num(row['24hRegisteredUser']);
    addStatus(status, row.ChargerStatus);
    hasType = addTypes(types, row.ChargerType) || hasType;
    ranking.push({
      name: regionLabel(row),
      orders: num(row['24hChargedOrder']),
      revenue: num(row['24hTotalIncome']),
      users: num(row['24hRegisteredUser']),
    });
  }

  ranking.sort((a, b) => b.orders - a.orders || b.revenue - a.revenue);

  const chargerTotal = status.online + status.offline + status.broken;
  if (!hasType && chargerTotal > 0) {
    types.fast = Math.round(chargerTotal * 0.65);
    types.slow = chargerTotal - types.fast;
  }

  const view = {
    datetime: report?.datetime || '',
    reportId: report?.ReportID || '',
    timezone: report?.timezone || 'GMT+8',
    regionCount: rows.length,
    metrics: {
      orders,
      revenue,
      online: status.online + status.offline,
      idle: status.online,
      busy: status.offline,
      broken: status.broken,
      users,
      chargers: chargerTotal,
    },
    statusPie: [
      { name: '空闲', value: status.online, color: '#61e6b8' },
      { name: '使用中', value: status.offline, color: '#ffb347' },
      { name: '故障', value: status.broken, color: '#ff6b6b' },
    ],
    ranking,
    chargerTypes: { ...types, estimated: !hasType },
  };

  const history = rememberSnapshot(view);
  view.trend = buildTrend(view, history);
  view.heatmap = buildHeatmap(view, history);
  view.prediction = predictionPayload
    ? buildPredictionFromMl(predictionPayload)
    : buildPredictionFallback(view);
  return view;
}
