<template>
  <el-card>
    <template #header>
      <div class="card-header">
        <span>会诊管理</span>
        <el-button type="primary" @click="handleAdd">申请会诊</el-button>
      </div>
    </template>
    
    <el-table :data="consultations" border stripe>
      <el-table-column prop="id" label="ID" width="80" />
      <el-table-column prop="patient.realName" label="患者姓名" />
      <el-table-column prop="requestDoctor.realName" label="申请医生" />
      <el-table-column prop="targetDoctor.realName" label="会诊医生" />
      <el-table-column prop="consultationType" label="类型">
        <template #default="{ row }">
          {{ row.consultationType === 'INPATIENT' ? '住院' : '门诊' }}
        </template>
      </el-table-column>
      <el-table-column prop="consultationDate" label="会诊日期" />
      <el-table-column prop="status" label="状态">
        <template #default="{ row }">
          <el-tag v-if="row.status === 'PENDING'" type="warning">待处理</el-tag>
          <el-tag v-else-if="row.status === 'ACCEPTED'" type="info">已接受</el-tag>
          <el-tag v-else-if="row.status === 'COMPLETED'" type="success">已完成</el-tag>
          <el-tag v-else type="danger">已取消</el-tag>
        </template>
      </el-table-column>
      <el-table-column label="操作" width="300">
        <template #default="{ row }">
          <el-button size="small" @click="handleView(row)">查看</el-button>
          <el-button
            v-if="row.status === 'PENDING' && isTargetDoctor(row)"
            size="small"
            type="success"
            @click="handleAccept(row)"
          >
            接受
          </el-button>
          <el-button
            v-if="row.status === 'ACCEPTED' && isTargetDoctor(row)"
            size="small"
            type="primary"
            @click="handleComplete(row)"
          >
            完成
          </el-button>
          <el-button
            v-if="row.status === 'PENDING' || row.status === 'ACCEPTED'"
            size="small"
            type="danger"
            @click="handleCancel(row)"
          >
            取消
          </el-button>
        </template>
      </el-table-column>
    </el-table>
    
    <el-dialog v-model="dialogVisible" title="申请会诊" width="600px">
      <el-form :model="form" label-width="100px">
        <el-form-item label="患者">
          <el-select v-model="form.patientId" filterable>
            <el-option
              v-for="patient in patients"
              :key="patient.userId"
              :label="patient.user.realName"
              :value="patient.userId"
            />
          </el-select>
        </el-form-item>
        <el-form-item label="会诊医生">
          <el-select v-model="form.targetDoctorId" filterable>
            <el-option
              v-for="doctor in doctors"
              :key="doctor.userId"
              :label="`${doctor.user.realName} - ${doctor.department}`"
              :value="doctor.userId"
            />
          </el-select>
        </el-form-item>
        <el-form-item label="会诊类型">
          <el-radio-group v-model="form.consultationType">
            <el-radio label="INPATIENT">住院</el-radio>
            <el-radio label="OUTPATIENT">门诊</el-radio>
          </el-radio-group>
        </el-form-item>
        <el-form-item label="会诊日期">
          <el-date-picker
            v-model="form.consultationDate"
            type="datetime"
            value-format="YYYY-MM-DD HH:mm:ss"
            style="width: 100%"
          />
        </el-form-item>
        <el-form-item label="会诊原因">
          <el-input v-model="form.consultationReason" type="textarea" :rows="4" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="dialogVisible = false">取消</el-button>
        <el-button type="primary" @click="handleSubmit">确定</el-button>
      </template>
    </el-dialog>
    
    <el-dialog v-model="completeVisible" title="完成会诊" width="600px">
      <el-form :model="completeForm" label-width="100px">
        <el-form-item label="会诊结果">
          <el-input v-model="completeForm.result" type="textarea" :rows="6" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="completeVisible = false">取消</el-button>
        <el-button type="primary" @click="handleCompleteSubmit">确定</el-button>
      </template>
    </el-dialog>
    
    <el-dialog v-model="viewVisible" title="会诊详情" width="800px">
      <el-descriptions :column="2" border>
        <el-descriptions-item label="患者姓名">{{ currentConsultation.patient?.realName }}</el-descriptions-item>
        <el-descriptions-item label="申请医生">{{ currentConsultation.requestDoctor?.realName }}</el-descriptions-item>
        <el-descriptions-item label="会诊医生">{{ currentConsultation.targetDoctor?.realName }}</el-descriptions-item>
        <el-descriptions-item label="会诊类型">
          {{ currentConsultation.consultationType === 'INPATIENT' ? '住院' : '门诊' }}
        </el-descriptions-item>
        <el-descriptions-item label="会诊日期">{{ currentConsultation.consultationDate }}</el-descriptions-item>
        <el-descriptions-item label="状态">
          <el-tag v-if="currentConsultation.status === 'PENDING'" type="warning">待处理</el-tag>
          <el-tag v-else-if="currentConsultation.status === 'ACCEPTED'" type="info">已接受</el-tag>
          <el-tag v-else-if="currentConsultation.status === 'COMPLETED'" type="success">已完成</el-tag>
          <el-tag v-else type="danger">已取消</el-tag>
        </el-descriptions-item>
        <el-descriptions-item label="会诊原因" :span="2">{{ currentConsultation.consultationReason }}</el-descriptions-item>
        <el-descriptions-item label="会诊结果" :span="2">{{ currentConsultation.consultationResult }}</el-descriptions-item>
      </el-descriptions>
    </el-dialog>
  </el-card>
</template>

<script setup>
import { ref, onMounted, computed } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { useUserStore } from '../../stores/user'
import api from '../../utils/api'

const consultations = ref([])
const patients = ref([])
const doctors = ref([])
const dialogVisible = ref(false)
const completeVisible = ref(false)
const viewVisible = ref(false)
const form = ref({})
const completeForm = ref({})
const currentConsultation = ref({})
const currentConsultationId = ref(null)
const userStore = useUserStore()

const loadConsultations = async () => {
  try {
    let res
    const user = JSON.parse(localStorage.getItem('user') || '{}')
    if (userStore.isDoctor()) {
      res = await api.get(`/consultation/doctor/${user.id}`)
    } else if (userStore.isPatient()) {
      res = await api.get(`/consultation/patient/${user.id}`)
    } else {
      // 管理员可以查看所有会诊
      res = await api.get('/consultation/list')
    }
    consultations.value = res.data || []
  } catch (error) {
    ElMessage.error('加载会诊列表失败')
  }
}

const loadPatients = async () => {
  try {
    const res = await api.get('/patient/list')
    patients.value = res.data || []
  } catch (error) {
    console.error('加载患者列表失败', error)
  }
}

const loadDoctors = async () => {
  try {
    const res = await api.get('/doctor/list')
    doctors.value = res.data || []
  } catch (error) {
    console.error('加载医生列表失败', error)
  }
}

const isTargetDoctor = (consultation) => {
  const user = JSON.parse(localStorage.getItem('user') || '{}')
  return consultation.targetDoctorId === user.id
}

const handleAdd = async () => {
  await loadPatients()
  await loadDoctors()
  const user = JSON.parse(localStorage.getItem('user') || '{}')
  form.value = { requestDoctorId: user.id, consultationType: 'OUTPATIENT' }
  dialogVisible.value = true
}

const handleView = (row) => {
  currentConsultation.value = row
  viewVisible.value = true
}

const handleAccept = async (row) => {
  try {
    await api.put(`/consultation/${row.id}/accept`)
    ElMessage.success('接受成功')
    loadConsultations()
  } catch (error) {
    ElMessage.error('操作失败')
  }
}

const handleComplete = (row) => {
  currentConsultationId.value = row.id
  completeForm.value = { result: row.consultationResult || '' }
  completeVisible.value = true
}

const handleCompleteSubmit = async () => {
  try {
    await api.put(`/consultation/${currentConsultationId.value}/complete`, completeForm.value)
    ElMessage.success('完成成功')
    completeVisible.value = false
    loadConsultations()
  } catch (error) {
    ElMessage.error('操作失败')
  }
}

const handleCancel = async (row) => {
  try {
    await ElMessageBox.confirm('确定要取消该会诊吗？', '提示', {
      type: 'warning'
    })
    await api.put(`/consultation/${row.id}/cancel`)
    ElMessage.success('取消成功')
    loadConsultations()
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('操作失败')
    }
  }
}

const handleSubmit = async () => {
  try {
    await api.post('/consultation', form.value)
    ElMessage.success('申请成功')
    dialogVisible.value = false
    loadConsultations()
  } catch (error) {
    ElMessage.error(error.message || '操作失败')
  }
}

onMounted(() => {
  loadConsultations()
})
</script>

<style scoped>
.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}
</style>

