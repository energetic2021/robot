<template>
  <div class="robot-control">
    <!-- 机器人控制主视图：集成监控、导航、遥控及自动化任务管理 -->
    <el-row :gutter="20">
      
      <!-- 1. ROS 服务连接状态 (仅管理员/医生可见) -->
      <el-col :span="24" v-if="!userStore.isPatient()">
        <el-card shadow="hover" class="card-health">
          <template #header>
            <span>ROS 服务状态</span>
            <el-button type="primary" link :loading="healthLoading" @click="checkHealth" style="float: right;">
              刷新
            </el-button>
          </template>
          <div class="health-content">
            <el-tag v-if="healthText" :type="healthOk ? 'success' : 'danger'" size="large">
              {{ healthOk ? '服务正常' : '服务异常' }}
            </el-tag>
            <pre v-if="healthText" class="health-pre">{{ healthText }}</pre>
          </div>
        </el-card>
      </el-col>

      <!-- 2. 实时状态监控 (仅管理员/医生可见) -->
      <el-col :xs="24" :md="12" v-if="!userStore.isPatient()">
        <el-card shadow="hover">
          <template #header>
            <span>机器人状态 (实时位姿)</span>
            <el-button type="primary" link :loading="statusLoading" @click="fetchStatus">刷新</el-button>
          </template>
          <pre v-if="statusJson" class="json-pre">{{ statusJson }}</pre>
          <el-empty v-else description="点击刷新获取状态" />
        </el-card>
      </el-col>
      <el-col :xs="24" :md="12" v-if="!userStore.isPatient()">
        <el-card shadow="hover">
          <template #header>
            <span>传感器数据 (激光雷达)</span>
            <el-button type="primary" link :loading="sensorLoading" @click="fetchSensor">刷新</el-button>
          </template>
          <pre v-if="sensorJson" class="json-pre">{{ sensorJson }}</pre>
          <el-empty v-else description="点击刷新获取传感器数据" />
        </el-card>
      </el-col>

      <!-- 3. 地图可视化：Canvas 渲染栅格地图与机器人位置 (仅管理员/医生可见) -->
      <el-col :span="24" v-if="!userStore.isPatient()">
        <el-card shadow="hover">
          <template #header>
            <span>实时地图监控</span>
            <el-button type="primary" link :loading="mapLoading" @click="fetchMap">刷新地图</el-button>
          </template>
          <div style="overflow:auto;text-align:center">
            <!-- 机器人位置以红色圆点标出，蓝色箭头代表朝向 -->
            <canvas ref="mapCanvas" width="500" height="500" style="border:1px solid #ccc;"></canvas>
          </div>
        </el-card>
      </el-col>

      <!-- 4. 手动移动控制面板 (仅管理员/医生可见) -->
      <el-col :span="24" v-if="!userStore.isPatient()">
        <el-card shadow="hover">
          <template #header>手动遥控面板</template>
          <div class="move-control">
            <div class="direction-row">
              <el-button :disabled="moveLoading" @click="move('forward')">前</el-button>
            </div>
            <div class="direction-row">
              <el-button :disabled="moveLoading" @click="move('left')">左</el-button>
              <el-button type="danger" :loading="stopLoading" @click="stop">停止</el-button>
              <el-button :disabled="moveLoading" @click="move('right')">右</el-button>
            </div>
            <div class="direction-row">
              <el-button :disabled="moveLoading" @click="move('backward')">后</el-button>
            </div>
            <div class="speed-row">
              <span>速度：</span>
              <el-slider v-model="speed" :min="0.1" :max="1" :step="0.1" style="width: 200px; display: inline-block; vertical-align: middle;" />
              <span>{{ speed }}</span>
            </div>
          </div>
        </el-card>
      </el-col>

      <!-- 5. 多点导航 (所有角色可见) -->
      <el-col :span="24">
        <el-card shadow="hover">
          <template #header>多点导航控制</template>
          <el-form inline>
            <el-form-item label="目标航点">
              <el-input v-model="waypointName" placeholder="如 1、2、3" style="width: 120px" />
            </el-form-item>
            <el-form-item>
              <el-button type="primary" :loading="navigateLoading" @click="navigateToWaypoint">开始导航</el-button>
            </el-form-item>
          </el-form>
          <p class="navi-tip">机器人将根据内置地图自动规划最优路径并前往目标点。</p>
        </el-card>
      </el-col>

      <!-- 6. 硬件抽屉控制 (仅管理员/医生可见) -->
      <el-col :span="24" v-if="!userStore.isPatient()">
        <el-card shadow="hover">
          <template #header>机器人抽屉控制</template>
          <div class="drawer-control">
            <el-button :loading="drawerLoading" @click="drawerAction('openAll')">全部开启</el-button>
            <el-button :loading="drawerLoading" @click="drawerAction('closeAll')">全部关闭</el-button>
            <el-divider direction="vertical" />
            <el-button :loading="drawerLoading" @click="drawerAction(1)">打开 1 号</el-button>
            <el-button :loading="drawerLoading" @click="drawerAction(2)">打开 2 号</el-button>
            <el-button :loading="drawerLoading" @click="drawerAction(3)">打开 3 号</el-button>
          </div>
        </el-card>
      </el-col>

      <!-- 7. 取物送物任务下发 (所有角色可见) -->
      <el-col :span="24">
        <el-card shadow="hover">
          <template #header>自动化取物送物任务</template>
          <el-form inline>
            <el-form-item label="取物地点">
              <el-input v-model="pickupPoint" placeholder="如 药房" style="width: 120px" />
            </el-form-item>
            <el-form-item label="送物地点">
              <el-input v-model="deliveryPoint" placeholder="如 102房" style="width: 120px" />
            </el-form-item>
            <el-form-item>
              <el-button type="primary" :loading="transportLoading" @click="startTransportTask">
                启动任务
              </el-button>
              <el-button :loading="transportCancelLoading" @click="cancelTransportTask">
                取消当前任务
              </el-button>
            </el-form-item>
          </el-form>
        </el-card>
      </el-col>

      <!-- 8. 任务队列与实时状态追踪 (所有角色可见) -->
      <el-col :span="24">
        <el-card shadow="hover">
          <template #header>
            <div class="status-header">
              <span>任务执行状态追踪</span>
              <el-button type="primary" link :loading="statusLoadingTransport" @click="refreshTransportStatus">
                手动刷新状态
              </el-button>
            </div>
          </template>
          <div class="transport-status">
            <h4>队列状态 (Action Status)</h4>
            <pre class="json-pre" v-if="transportStatusText">{{ transportStatusText }}</pre>
            <el-empty v-else description="暂无任务在队列中" />

            <h4 style="margin-top: 16px;">实时执行反馈 (Action Feedback)</h4>
            <pre class="json-pre" v-if="transportFeedbackText">{{ transportFeedbackText }}</pre>
            <el-empty v-else description="暂无执行中的反馈" />

            <h4 style="margin-top: 16px;">最近执行结果 (Action Result)</h4>
            <pre class="json-pre" v-if="transportResultText">{{ transportResultText }}</pre>
            <el-empty v-else description="暂无已完成的任务结果" />
          </div>
        </el-card>
      </el-col>
    </el-row>
  </div>
</template>

<script setup>
/**
 * 机器人控制逻辑：
 * 1. 通过 axios 调用后端 REST API。
 * 2. 核心控制逻辑包括 WebSocket 状态检查、RPC 调用、话题发布。
 * 3. 复杂渲染：利用 HTML5 Canvas 渲染 ROS 栅格地图和机器人实时位姿。
 */
import { ref, onMounted, onUnmounted, nextTick } from 'vue'
import { ElMessage } from 'element-plus'
import { 
  robotHealth, robotStatus, robotSensor, robotAdd, robotMove, robotStop, 
  robotNavigate, robotDrawersOpenAll, robotDrawersCloseAll, robotDrawerOpen, 
  robotTransportStart, robotTransportCancel, robotTransportStatus, 
  robotTransportFeedback, robotTransportResult, robotMap 
} from '@/api/robot'
import { useUserStore } from '@/stores/user'

const userStore = useUserStore()

// --- 响应式状态变量 ---
const healthLoading = ref(false)
const healthText = ref('')
const healthOk = ref(false)

const statusLoading = ref(false)
const statusJson = ref('')

const sensorLoading = ref(false)
const sensorJson = ref('')

const speed = ref(0.5)
const moveLoading = ref(false)
const stopLoading = ref(false)

const waypointName = ref('1')
const navigateLoading = ref(false)

const drawerLoading = ref(false)

const pickupPoint = ref('1') // 默认取物点设为药房(1)
const deliveryPoint = ref('2')
const transportLoading = ref(false)
const transportCancelLoading = ref(false)

const statusLoadingTransport = ref(false)
const transportStatusText = ref('')
const transportFeedbackText = ref('')
const transportResultText = ref('')

// --- 地图与可视化相关 ---
const mapLoading = ref(false)
const mapData = ref(null)
const mapMeta = ref(null)
const robotPose = ref(null)
const mapCanvas = ref(null)

/**
 * 核心渲染逻辑：将 ROS 栅格地图数据绘制到 Canvas
 */
function drawMapAndRobot() {
  const canvas = mapCanvas.value
  if (!canvas || !mapData.value || !mapMeta.value) return
  
  const ctx = canvas.getContext('2d')
  ctx.clearRect(0, 0, canvas.width, canvas.height)
  
  const { width, height, resolution, origin } = mapMeta.value
  const data = mapData.value
  
  // 计算缩放比例，使地图适应 Canvas 大小
  const scaleX = canvas.width / width
  const scaleY = canvas.height / height
  
  // 1. 绘制地图栅格：100 代表障碍物（黑色），0 代表自由空间（白色），-1 代表未知（灰色）
  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      const idx = y * width + x
      const val = data[idx]
      let color = '#fff'
      if (val === 100) color = '#222'
      else if (val > 0 && val < 100) color = '#aaa'
      ctx.fillStyle = color
      ctx.fillRect(x * scaleX, y * scaleY, scaleX, scaleY)
    }
  }
  
  // 2. 绘制机器人图标
  if (robotPose.value) {
    const { position, orientation } = robotPose.value.pose.pose
    // 坐标系转换：ROS (米) -> 栅格 (像素) -> Canvas (像素)
    const px = (position.x - origin.x) / resolution
    const py = height - (position.y - origin.y) / resolution
    
    ctx.save()
    ctx.beginPath()
    ctx.arc(px * scaleX, py * scaleY, 8, 0, 2 * Math.PI)
    ctx.fillStyle = 'red'
    ctx.fill()
    ctx.strokeStyle = '#000'
    ctx.stroke()
    
    // 绘制朝向箭头（通过四元数转换得到弧度）
    ctx.beginPath()
    const theta = 2 * Math.atan2(orientation.z, orientation.w)
    ctx.moveTo(px * scaleX, py * scaleY)
    ctx.lineTo((px + 12 * Math.cos(theta)) * scaleX, (py - 12 * Math.sin(theta)) * scaleY)
    ctx.strokeStyle = 'blue'
    ctx.lineWidth = 2
    ctx.stroke()
    ctx.restore()
  }
}

/** 获取最新的静态地图数据 */
const fetchMap = async () => {
  mapLoading.value = true
  try {
    const map = await robotMap()
    if (typeof map === 'string') return
    mapData.value = map.data
    mapMeta.value = map.info
    await nextTick()
    drawMapAndRobot()
  } finally {
    mapLoading.value = false
  }
}

/** 获取机器人实时位姿数据并触发重绘 */
const fetchRobotPose = async () => {
  try {
    const status = await robotStatus()
    let poseObj = typeof status === 'string' ? JSON.parse(status) : status
    if (poseObj && poseObj.pose) {
      robotPose.value = poseObj
      drawMapAndRobot()
    }
  } catch {}
}

/** 检查后端与 ROS 之间的连接健康状况 */
const checkHealth = async () => {
  healthLoading.value = true
  try {
    const data = await robotHealth()
    healthText.value = typeof data === 'string' ? data : JSON.stringify(data, null, 2)
    healthOk.value = healthText.value.includes('✅')
  } catch (e) {
    healthText.value = '连接超时或服务未启动'
    healthOk.value = false
  } finally {
    healthLoading.value = false
  }
}

/** 接口：获取原始状态 JSON */
const fetchStatus = async () => {
  statusLoading.value = true
  try {
    const data = await robotStatus()
    statusJson.value = JSON.stringify(typeof data === 'string' ? JSON.parse(data) : data, null, 2)
  } catch (e) {
    statusJson.value = '获取失败'
  } finally {
    statusLoading.value = false
  }
}

/** 接口：手动遥控指令下发 */
const move = async (direction) => {
  moveLoading.value = true
  try {
    const data = await robotMove(direction, speed.value)
    if (data && data.success) ElMessage.success('移动指令已下发')
  } finally {
    moveLoading.value = false
  }
}

/** 接口：停止指令 */
const stop = async () => {
  stopLoading.value = true
  try {
    await robotStop()
    ElMessage.warning('机器人已紧急停止')
  } finally {
    stopLoading.value = false
  }
}

/** 接口：启动自动化运输任务 */
const startTransportTask = async () => {
  transportLoading.value = true
  try {
    const data = await robotTransportStart(pickupPoint.value, deliveryPoint.value)
    if (data.success) ElMessage.success('任务已加入机器人队列')
  } finally {
    transportLoading.value = false
  }
}

/** 接口：刷新任务状态追踪数据（Status/Feedback/Result） */
const refreshTransportStatus = async () => {
  statusLoadingTransport.value = true
  try {
    const [st, fb, rs] = await Promise.allSettled([
      robotTransportStatus(),
      robotTransportFeedback(),
      robotTransportResult()
    ])
    transportStatusText.value = st.status === 'fulfilled' ? JSON.stringify(st.value, null, 2) : ''
    transportFeedbackText.value = fb.status === 'fulfilled' ? JSON.stringify(fb.value, null, 2) : ''
    transportResultText.value = rs.status === 'fulfilled' ? JSON.stringify(rs.value, null, 2) : ''
  } finally {
    statusLoadingTransport.value = false
  }
}

// --- 生命周期钩子 ---
onMounted(async () => {
  // 仅对专业人员（管理员/医生）开启自动刷新和地图显示
  if (!userStore.isPatient()) {
    await fetchMap()
    await fetchRobotPose()
    checkHealth()
  }
})

</script>

<style scoped>
.robot-control { padding: 0; }
.health-content { min-height: 60px; }
.health-pre { margin-top: 12px; font-size: 13px; color: #606266; white-space: pre-wrap; }
.json-pre { font-size: 12px; max-height: 280px; overflow: auto; background: #f5f7fa; padding: 12px; border-radius: 4px; }
.move-control { text-align: center; }
.direction-row { margin-bottom: 10px; }
.speed-row { margin-top: 16px; font-size: 14px; }
.navi-tip { margin-top: 12px; font-size: 12px; color: #909399; }
.drawer-control { display: flex; flex-wrap: wrap; gap: 10px; }
.status-header { display: flex; justify-content: space-between; align-items: center; }
</style>
