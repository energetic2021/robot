import axios from 'axios'
import { ElMessage } from 'element-plus'
import router from '../router'

/**
 * 机器人/ROS 接口（返回原始 JSON，不校验 code）
 */
const robotApi = axios.create({
  baseURL: '/api',
  timeout: 10000,
  headers: { 'Content-Type': 'application/json' }
})

robotApi.interceptors.request.use(
  config => {
    const token = localStorage.getItem('token')
    if (token) {
      config.headers.Authorization = `Bearer ${token}`
    }
    return config
  },
  error => Promise.reject(error)
)

robotApi.interceptors.response.use(
  response => response.data,
  error => {
    if (error.response?.status === 401) {
      localStorage.removeItem('token')
      localStorage.removeItem('user')
      router.push('/login')
      ElMessage.error('登录已过期，请重新登录')
    } else {
      ElMessage.error(error.response?.data?.message || error.message || '网络错误')
    }
    return Promise.reject(error)
  }
)

export const robotHealth = () => robotApi.get('/robot/health')
export const robotStatus = () => robotApi.get('/robot/status')
export const robotSensor = () => robotApi.get('/robot/sensor')
export const robotAdd = (a, b) => robotApi.get('/robot/add', { params: { a, b } })
export const robotMove = (direction, speed = 0.5) =>
  robotApi.post('/robot/move', null, { params: { direction, speed } })
export const robotStop = () => robotApi.post('/robot/stop')
/** 发布导航航点到 /waterplus/navi_waypoint，waypoint 为航点名称如 "1"、"2" */
export const robotNavigate = (waypoint) =>
  robotApi.post('/robot/navigate', null, { params: { waypoint } })

/** 抽屉控制：对应 rosservice call /drawers/open_all、close_all、open_drawer1~3 */
export const robotDrawersOpenAll = () => robotApi.post('/robot/drawers/open-all')
export const robotDrawersCloseAll = () => robotApi.post('/robot/drawers/close-all')
export const robotDrawerOpen = (num) => robotApi.post(`/robot/drawers/open/${num}`)

/** 取物送物任务：发送 Action goal 与取消 */
export const robotTransportStart = (pickupPoint, deliveryPoint) =>
  robotApi.post('/robot/transport/start', null, { params: { pickupPoint, deliveryPoint } })
export const robotTransportCancel = () => robotApi.post('/robot/transport/cancel')

/** 取物送物任务状态查询：status / feedback / result */
export const robotTransportStatus = () => robotApi.get('/robot/transport/status')
export const robotTransportFeedback = () => robotApi.get('/robot/transport/feedback')
export const robotTransportResult = () => robotApi.get('/robot/transport/result')

// 获取地图数据接口，返回地图的 JSON 数据
export const robotMap = () => robotApi.get('/robot/map')


export default robotApi
