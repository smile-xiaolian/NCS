const axisLine = { lineStyle: { color: 'rgba(85, 220, 255, 0.28)' } };
const splitLine = { lineStyle: { color: 'rgba(21, 139, 196, 0.18)' } };
const textColor = '#9ec9dc';

function tooltipBase() {
  return {
    backgroundColor: 'rgba(5, 18, 36, 0.94)',
    borderColor: '#158bc4',
    borderWidth: 1,
    textStyle: { color: '#d8f3ff', fontSize: 13 },
  };
}

function legendBase(extra = {}) {
  return {
    textStyle: { color: textColor },
    icon: 'circle',
    itemWidth: 10,
    itemHeight: 10,
    selectedMode: true,
    ...extra,
  };
}

export function pieOption(items) {
  const total = items.reduce((sum, item) => sum + item.value, 0);
  return {
    color: items.map((item) => item.color),
    tooltip: {
      ...tooltipBase(),
      trigger: 'item',
      formatter: ({ name, value, percent }) => `${name}<br/>数量 ${value}（${percent}%）`,
    },
    legend: legendBase({ bottom: 8, left: 'center' }),
    series: [
      {
        type: 'pie',
        radius: ['42%', '68%'],
        center: ['50%', '46%'],
        avoidLabelOverlap: true,
        itemStyle: {
          borderColor: '#07192e',
          borderWidth: 3,
        },
        label: {
          color: '#d8f3ff',
          formatter: '{b}\n{d}%',
        },
        data: items.map((item) => ({ name: item.name, value: item.value })),
      },
    ],
    graphic: total
      ? [
          {
            type: 'text',
            left: 'center',
            top: '42%',
            style: {
              text: String(total),
              fill: '#55dcff',
              fontSize: 22,
              fontWeight: 700,
              textAlign: 'center',
            },
          },
          {
            type: 'text',
            left: 'center',
            top: '50%',
            style: {
              text: '电桩总数',
              fill: textColor,
              fontSize: 12,
              textAlign: 'center',
            },
          },
        ]
      : [],
  };
}

export function rankOption(ranking) {
  const top = ranking.slice(0, 8).slice().reverse();
  return {
    tooltip: {
      ...tooltipBase(),
      trigger: 'axis',
      axisPointer: { type: 'shadow' },
      formatter: (rows) => {
        const row = ranking.find((item) => item.name === rows?.[0]?.name);
        if (!row) return '';
        return `${row.name}<br/>近24h订单 ${row.orders}<br/>近24h营收 ¥ ${row.revenue.toFixed(2)}`;
      },
    },
    grid: { left: 88, right: 36, top: 16, bottom: 16 },
    xAxis: {
      type: 'value',
      axisLine,
      splitLine,
      axisLabel: { color: textColor },
    },
    yAxis: {
      type: 'category',
      data: top.map((item) => item.name),
      axisLine,
      axisLabel: { color: '#d8f3ff', fontSize: 12 },
      splitLine: { show: false },
    },
    series: [
      {
        type: 'bar',
        data: top.map((item) => item.orders),
        barWidth: 14,
        itemStyle: {
          borderRadius: [0, 8, 8, 0],
          color: {
            type: 'linear',
            x: 0,
            y: 0,
            x2: 1,
            y2: 0,
            colorStops: [
              { offset: 0, color: '#0e6f9a' },
              { offset: 1, color: '#55dcff' },
            ],
          },
        },
        label: {
          show: true,
          position: 'right',
          color: '#9eefff',
          formatter: '{c}',
        },
      },
    ],
  };
}

export function trendOption(trend) {
  return {
    tooltip: {
      ...tooltipBase(),
      trigger: 'axis',
      formatter: (rows) => {
        const point = rows?.[0];
        if (!point) return '';
        const item = trend.points[point.dataIndex];
        const tag = item?.real ? '实测快照' : '周规律外推';
        return `${point.axisValue}<br/>营收 ¥ ${Number(point.value).toFixed(2)}<br/>${tag}`;
      },
    },
    legend: legendBase({ top: 4, right: 12, data: ['营收'] }),
    grid: { left: 56, right: 22, top: 42, bottom: 32 },
    xAxis: {
      type: 'category',
      data: trend.points.map((item) => item.day),
      axisLine,
      axisLabel: { color: textColor, rotate: 30 },
    },
    yAxis: {
      type: 'value',
      name: '元',
      nameTextStyle: { color: textColor },
      axisLine,
      splitLine,
      axisLabel: { color: textColor },
    },
    series: [
      {
        name: '营收',
        type: 'line',
        smooth: true,
        symbol: 'circle',
        symbolSize: 6,
        data: trend.points.map((item) => item.revenue),
        lineStyle: { width: 2, color: '#55dcff' },
        itemStyle: { color: '#61e6b8' },
        areaStyle: {
          color: {
            type: 'linear',
            x: 0,
            y: 0,
            x2: 0,
            y2: 1,
            colorStops: [
              { offset: 0, color: 'rgba(85, 220, 255, 0.35)' },
              { offset: 1, color: 'rgba(85, 220, 255, 0.02)' },
            ],
          },
        },
      },
    ],
  };
}

export function heatOption(heatmap) {
  return {
    tooltip: {
      ...tooltipBase(),
      position: 'top',
      formatter: ({ data }) => {
        const [hour, weekday, value] = data;
        return `${heatmap.weekdays[weekday]} ${heatmap.hours[hour]}<br/>负荷 ${value}`;
      },
    },
    grid: { left: 52, right: 18, top: 16, bottom: 52 },
    xAxis: {
      type: 'category',
      data: heatmap.hours,
      splitArea: { show: true },
      axisLabel: { color: textColor, interval: 1, fontSize: 10 },
      axisLine,
    },
    yAxis: {
      type: 'category',
      data: heatmap.weekdays,
      splitArea: { show: true },
      axisLabel: { color: '#d8f3ff' },
      axisLine,
    },
    visualMap: {
      min: 0,
      max: heatmap.max,
      calculable: true,
      orient: 'horizontal',
      left: 'center',
      bottom: 4,
      text: ['高', '低'],
      textStyle: { color: textColor },
      inRange: { color: ['#082036', '#0e6f9a', '#55dcff', '#f6e05e'] },
    },
    series: [
      {
        type: 'heatmap',
        data: heatmap.cells,
        emphasis: { itemStyle: { shadowBlur: 8, shadowColor: 'rgba(85, 220, 255, 0.6)' } },
      },
    ],
  };
}

export function ringOption(types) {
  const items = [
    { name: '快充', value: types.fast, color: '#55dcff' },
    { name: '慢充', value: types.slow, color: '#ffb347' },
  ];
  return {
    color: items.map((item) => item.color),
    tooltip: {
      ...tooltipBase(),
      trigger: 'item',
      formatter: ({ name, value, percent }) => `${name}<br/>${value} 台（${percent}%）`,
    },
    legend: legendBase({ bottom: 6, left: 'center' }),
    series: [
      {
        type: 'pie',
        radius: ['50%', '72%'],
        center: ['50%', '46%'],
        label: {
          color: '#d8f3ff',
          formatter: '{b} {d}%',
        },
        itemStyle: { borderColor: '#07192e', borderWidth: 3 },
        data: items,
      },
    ],
  };
}

export function predictOption(prediction) {
  const points = Array.isArray(prediction) ? prediction : prediction?.points || [];
  const history = Array.isArray(prediction) ? [] : prediction?.history || [];
  const empty = points.length === 0;
  const legend = ['预测负荷'];
  if (history.length) legend.unshift('历史实际');
  return {
    tooltip: {
      ...tooltipBase(),
      trigger: 'axis',
      formatter: (rows) => {
        if (!rows?.length) return '';
        const item = points[rows[0].dataIndex];
        const lines = rows.map((row) => `${row.seriesName} ${row.value} 度`);
        if (item?.idle) lines.push(`预测空闲 ${item.idle} 桩`);
        if (item?.peak) lines.push('高峰时段');
        return `${rows[0].axisValue}<br/>${lines.join('<br/>')}`;
      },
    },
    legend: legendBase({ top: 0, right: 8, data: legend }),
    grid: { left: 46, right: 18, top: 32, bottom: 28 },
    xAxis: {
      type: 'category',
      data: points.map((item) => item.label),
      axisLine,
      axisLabel: { color: textColor, fontSize: 10, interval: 1 },
    },
    yAxis: {
      type: 'value',
      name: '度',
      nameTextStyle: { color: textColor },
      axisLine,
      splitLine,
      axisLabel: { color: textColor },
    },
    graphic: empty
      ? [
          {
            type: 'text',
            left: 'center',
            top: 'middle',
            style: {
              text: prediction?.message || '暂无预测数据',
              fill: '#6f98ab',
              fontSize: 13,
              width: 360,
              overflow: 'break',
            },
          },
        ]
      : [],
    series: [
      history.length
        ? {
            name: '历史实际',
            type: 'line',
            smooth: true,
            data: history.map((item) => item.load),
            lineStyle: { width: 2, color: '#55dcff' },
            itemStyle: { color: '#55dcff' },
          }
        : null,
      {
        name: '预测负荷',
        type: 'line',
        smooth: true,
        data: points.map((item) => item.load),
        lineStyle: { width: 2, color: '#61e6b8' },
        itemStyle: { color: '#61e6b8' },
        areaStyle: { color: 'rgba(97, 230, 184, 0.16)' },
        markPoint: {
          data: points
            .map((item) => (item.peak ? { coord: [item.label, item.load], value: '峰', itemStyle: { color: '#ff6b6b' } } : null))
            .filter(Boolean)
            .filter((_item, index) => index % 2 === 0),
          label: { color: '#fff', fontSize: 10 },
        },
      },
    ].filter(Boolean),
  };
}
