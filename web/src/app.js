import { nextTick, onBeforeUnmount, onMounted, ref } from 'vue';
import * as echarts from 'echarts';
import {
  DATA_URL,
  PRED_URL,
  REFRESH_MS,
  aggregate,
  formatClock,
  formatInt,
  formatMoney,
} from './aggregate.js';
import {
  heatOption,
  pieOption,
  predictOption,
  rankOption,
  ringOption,
  trendOption,
} from './charts.js';

const DESIGN = { width: 1920, height: 1080 };

export default {
  setup() {
    const screen = ref(null);
    const clock = ref('--');
    const stamp = ref('等待数据');
    const loadError = ref('');
    const view = ref(aggregate({ data: [] }));
    const pieEl = ref(null);
    const rankEl = ref(null);
    const trendEl = ref(null);
    const heatEl = ref(null);
    const typeEl = ref(null);
    const predEl = ref(null);

    const charts = [];
    let clockTimer = 0;
    let refreshTimer = 0;

    function fitScreen() {
      const el = screen.value;
      if (!el) return;
      const scale = Math.min(window.innerWidth / DESIGN.width, window.innerHeight / DESIGN.height);
      el.style.transform = `scale(${scale})`;
    }

    function resizeCharts() {
      fitScreen();
      charts.forEach((chart) => chart.resize());
    }

    function bindChart(el) {
      const chart = echarts.init(el);
      charts.push(chart);
    }

    function renderCharts() {
      const data = view.value;
      charts[0]?.setOption(pieOption(data.statusPie), true);
      charts[1]?.setOption(rankOption(data.ranking), true);
      charts[2]?.setOption(trendOption(data.trend), true);
      charts[3]?.setOption(heatOption(data.heatmap), true);
      charts[4]?.setOption(ringOption(data.chargerTypes), true);
      charts[5]?.setOption(predictOption(data.prediction), true);
    }

    async function fetchJson(url) {
      const response = await fetch(`${url}?t=${Date.now()}`);
      if (!response.ok) throw new Error(`HTTP ${response.status}`);
      return response.json();
    }

    async function load() {
      try {
        const report = await fetchJson(DATA_URL);
        let prediction = null;
        try {
          prediction = await fetchJson(PRED_URL);
        } catch {
          prediction = null;
        }
        view.value = aggregate(report, prediction);
        stamp.value = view.value.datetime || '未知时间';
        loadError.value = '';
        renderCharts();
        await nextTick();
        charts.forEach((chart) => chart.resize());
      } catch (error) {
        loadError.value = '未读到数据文件，请将 webmodel 格式 JSON 放到 public/data/dashboard.json';
        if (!view.value.datetime) stamp.value = '暂无数据';
        console.warn(error);
      }
    }

    onMounted(async () => {
      clock.value = formatClock(new Date());
      clockTimer = window.setInterval(() => {
        clock.value = formatClock(new Date());
      }, 1000);

      bindChart(pieEl.value);
      bindChart(rankEl.value);
      bindChart(trendEl.value);
      bindChart(heatEl.value);
      bindChart(typeEl.value);
      bindChart(predEl.value);

      fitScreen();
      await nextTick();
      charts.forEach((chart) => chart.resize());
      load();
      refreshTimer = window.setInterval(load, REFRESH_MS);
      window.addEventListener('resize', resizeCharts);
    });

    onBeforeUnmount(() => {
      window.clearInterval(clockTimer);
      window.clearInterval(refreshTimer);
      window.removeEventListener('resize', resizeCharts);
      charts.forEach((chart) => chart.dispose());
    });

    return {
      screen,
      clock,
      stamp,
      loadError,
      view,
      pieEl,
      rankEl,
      trendEl,
      heatEl,
      typeEl,
      predEl,
      formatInt,
      formatMoney,
    };
  },
  template: `
    <div class="stage">
      <div class="screen" ref="screen">
        <header class="topbar">
          <div class="topbar-side">
            <span>Report {{ view.reportId || '--' }}</span>
            <span>{{ view.timezone }}</span>
            <span>{{ view.regionCount }} 个区域</span>
          </div>
          <h1>东软电动汽车充电桩应用管理平台</h1>
          <div class="topbar-side right">
            <time>{{ clock }}</time>
          </div>
        </header>

        <p class="meta">
          数据截止：{{ stamp }}
          <span>口径：各区域近 24 小时汇总</span>
          <span>刷新周期：1 小时</span>
          <span class="warn" v-if="loadError">{{ loadError }}</span>
        </p>

        <div class="body">
          <div class="col col-left">
            <section class="panel">
              <h2>电桩状态占比</h2>
              <div class="chart" ref="pieEl"></div>
            </section>
            <section class="panel">
              <h2>区域充电量排行</h2>
              <div class="chart" ref="rankEl"></div>
            </section>
          </div>

          <div class="col col-center">
            <section class="metrics">
              <article>
                <small>总充电次数</small>
                <b>{{ formatInt(view.metrics.orders) }}</b>
                <em>近 24 小时订单</em>
              </article>
              <article>
                <small>总营收</small>
                <b>¥ {{ formatMoney(view.metrics.revenue) }}</b>
                <em>近 24 小时结算</em>
              </article>
              <article>
                <small>在线电桩数</small>
                <b>{{ formatInt(view.metrics.online) }}</b>
                <em>空闲 {{ formatInt(view.metrics.idle) }} / 使用中 {{ formatInt(view.metrics.busy) }}</em>
              </article>
              <article>
                <small>注册用户数</small>
                <b>{{ formatInt(view.metrics.users) }}</b>
                <em>近 24 小时新增</em>
              </article>
            </section>
            <section class="panel">
              <h2>
                近 30 日营收趋势
                <i v-if="view.trend.estimated">连续快照不足时按周规律外推，接入小时级 JSON 后自动替换</i>
              </h2>
              <div class="chart" ref="trendEl"></div>
            </section>
          </div>

          <div class="col col-right">
            <section class="panel">
              <h2>
                充电时段热力分布
                <i v-if="view.heatmap.estimated">由近 24h 订单按典型峰谷分摊</i>
              </h2>
              <div class="chart" ref="heatEl"></div>
            </section>
            <section class="panel">
              <h2>
                快充 / 慢充占比
                <i v-if="view.chargerTypes.estimated">上报未含类型时按混合桩经验配比</i>
              </h2>
              <div class="chart" ref="typeEl"></div>
            </section>
            <section class="panel">
              <h2>
                未来 24 小时负荷预测
                <i v-if="view.prediction.source === 'ml'">
                  模型输出{{ view.prediction.metrics && view.prediction.metrics.mae != null ? '，MAE ' + view.prediction.metrics.mae : '' }}，高峰标红
                </i>
                <i v-else-if="view.prediction.source === 'empty'">{{ view.prediction.message }}</i>
                <i v-else>由近 24h 订单按峰谷外推，高峰标红</i>
              </h2>
              <div class="chart" ref="predEl"></div>
            </section>
          </div>
        </div>
      </div>
    </div>
  `,
};
