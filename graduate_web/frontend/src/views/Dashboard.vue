<template>
  <div class="dashboard">
    <el-row :gutter="20">
      <el-col :span="6">
        <el-card>
          <div class="stat-item">
            <div class="stat-value">医生总数</div>
            <div class="stat-number">{{ stats.doctors }}</div>
          </div>
        </el-card>
      </el-col>
      <el-col :span="6">
        <el-card>
          <div class="stat-item">
            <div class="stat-value">患者总数</div>
            <div class="stat-number">{{ stats.patients }}</div>
          </div>
        </el-card>
      </el-col>
      <el-col :span="6">
        <el-card>
          <div class="stat-item">
            <div class="stat-value">病历总数</div>
            <div class="stat-number">{{ stats.records }}</div>
          </div>
        </el-card>
      </el-col>
      <el-col :span="6">
        <el-card>
          <div class="stat-item">
            <div class="stat-value">会诊总数</div>
            <div class="stat-number">{{ stats.consultations }}</div>
          </div>
        </el-card>
      </el-col>
    </el-row>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import api from '../utils/api'

const stats = ref({
  doctors: 0,
  patients: 0,
  records: 0,
  consultations: 0
})

const loadStats = async () => {
  try {
    const [doctorsRes, patientsRes] = await Promise.all([
      api.get('/doctor/list'),
      api.get('/patient/list')
    ])
    stats.value.doctors = doctorsRes.data?.length || 0
    stats.value.patients = patientsRes.data?.length || 0
  } catch (error) {
    console.error('加载统计数据失败', error)
  }
}

onMounted(() => {
  loadStats()
})
</script>

<style scoped>
.dashboard {
  padding: 20px;
}

.stat-item {
  text-align: center;
}

.stat-value {
  font-size: 14px;
  color: #909399;
  margin-bottom: 10px;
}

.stat-number {
  font-size: 32px;
  font-weight: bold;
  color: #409EFF;
}
</style>

