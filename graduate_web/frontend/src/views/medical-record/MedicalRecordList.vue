<template>
  <el-card>
    <template #header>
      <div class="card-header">
        <span>病历管理</span>
        <el-button type="primary" @click="handleAdd">新建病历</el-button>
      </div>
    </template>
    
    <el-table :data="records" border stripe>
      <el-table-column prop="id" label="ID" width="80" />
      <el-table-column prop="patient.realName" label="患者姓名" />
      <el-table-column prop="doctor.realName" label="医生姓名" />
      <el-table-column prop="visitDate" label="就诊日期" />
      <el-table-column prop="diagnosis" label="诊断" />
      <el-table-column label="操作" width="250">
        <template #default="{ row }">
          <el-button size="small" @click="handleView(row)">查看</el-button>
          <el-button size="small" @click="handleEdit(row)">编辑</el-button>
          <el-button size="small" type="danger" @click="handleDelete(row)">删除</el-button>
        </template>
      </el-table-column>
    </el-table>
    
    <el-dialog v-model="dialogVisible" :title="dialogTitle" width="800px">
      <el-form :model="form" label-width="100px">
        <el-row :gutter="20">
          <el-col :span="12">
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
          </el-col>
          <el-col :span="12">
            <el-form-item label="就诊日期">
              <el-date-picker
                v-model="form.visitDate"
                type="datetime"
                value-format="YYYY-MM-DD HH:mm:ss"
                style="width: 100%"
              />
            </el-form-item>
          </el-col>
        </el-row>
        <el-form-item label="主诉">
          <el-input v-model="form.chiefComplaint" type="textarea" :rows="2" />
        </el-form-item>
        <el-form-item label="现病史">
          <el-input v-model="form.presentIllness" type="textarea" :rows="3" />
        </el-form-item>
        <el-form-item label="既往史">
          <el-input v-model="form.pastHistory" type="textarea" :rows="2" />
        </el-form-item>
        <el-form-item label="体格检查">
          <el-input v-model="form.physicalExam" type="textarea" :rows="3" />
        </el-form-item>
        <el-form-item label="诊断">
          <el-input v-model="form.diagnosis" type="textarea" :rows="2" />
        </el-form-item>
        <el-form-item label="治疗方案">
          <el-input v-model="form.treatmentPlan" type="textarea" :rows="3" />
        </el-form-item>
        <el-form-item label="备注">
          <el-input v-model="form.remarks" type="textarea" :rows="2" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="dialogVisible = false">取消</el-button>
        <el-button type="primary" @click="handleSubmit">确定</el-button>
      </template>
    </el-dialog>
    
    <el-dialog v-model="viewVisible" title="病历详情" width="800px">
      <el-descriptions :column="2" border>
        <el-descriptions-item label="患者姓名">{{ currentRecord.patient?.realName }}</el-descriptions-item>
        <el-descriptions-item label="医生姓名">{{ currentRecord.doctor?.realName }}</el-descriptions-item>
        <el-descriptions-item label="就诊日期">{{ currentRecord.visitDate }}</el-descriptions-item>
        <el-descriptions-item label="主诉" :span="2">{{ currentRecord.chiefComplaint }}</el-descriptions-item>
        <el-descriptions-item label="现病史" :span="2">{{ currentRecord.presentIllness }}</el-descriptions-item>
        <el-descriptions-item label="既往史" :span="2">{{ currentRecord.pastHistory }}</el-descriptions-item>
        <el-descriptions-item label="体格检查" :span="2">{{ currentRecord.physicalExam }}</el-descriptions-item>
        <el-descriptions-item label="诊断" :span="2">{{ currentRecord.diagnosis }}</el-descriptions-item>
        <el-descriptions-item label="治疗方案" :span="2">{{ currentRecord.treatmentPlan }}</el-descriptions-item>
        <el-descriptions-item label="备注" :span="2">{{ currentRecord.remarks }}</el-descriptions-item>
      </el-descriptions>
    </el-dialog>
  </el-card>
</template>

<script setup>
import { ref, onMounted, computed } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { useUserStore } from '../../stores/user'
import api from '../../utils/api'

const records = ref([])
const patients = ref([])
const dialogVisible = ref(false)
const viewVisible = ref(false)
const dialogTitle = ref('新建病历')
const form = ref({})
const currentRecord = ref({})
const userStore = useUserStore()

const loadRecords = async () => {
  try {
    let res
    if (userStore.isDoctor()) {
      const user = JSON.parse(localStorage.getItem('user') || '{}')
      res = await api.get(`/medical-record/doctor/${user.id}`)
    } else {
      res = await api.get('/medical-record/list')
    }
    records.value = res.data || []
  } catch (error) {
    ElMessage.error('加载病历列表失败')
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

const handleAdd = async () => {
  await loadPatients()
  const user = JSON.parse(localStorage.getItem('user') || '{}')
  form.value = { doctorId: user.id }
  dialogTitle.value = '新建病历'
  dialogVisible.value = true
}

const handleView = (row) => {
  currentRecord.value = row
  viewVisible.value = true
}

const handleEdit = async (row) => {
  await loadPatients()
  form.value = { ...row, patientId: row.patientId }
  dialogTitle.value = '编辑病历'
  dialogVisible.value = true
}

const handleSubmit = async () => {
  try {
    if (form.value.id) {
      await api.put(`/medical-record/${form.value.id}`, form.value)
      ElMessage.success('更新成功')
    } else {
      await api.post('/medical-record', form.value)
      ElMessage.success('创建成功')
    }
    dialogVisible.value = false
    loadRecords()
  } catch (error) {
    ElMessage.error(error.message || '操作失败')
  }
}

const handleDelete = async (row) => {
  try {
    await ElMessageBox.confirm('确定要删除该病历吗？', '提示', {
      type: 'warning'
    })
    await api.delete(`/medical-record/${row.id}`)
    ElMessage.success('删除成功')
    loadRecords()
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('删除失败')
    }
  }
}

onMounted(() => {
  loadRecords()
})
</script>

<style scoped>
.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}
</style>

