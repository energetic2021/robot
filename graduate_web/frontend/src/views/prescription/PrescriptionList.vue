<template>
  <el-card>
    <template #header>
      <div class="card-header">
        <span>处方管理</span>
        <el-button type="primary" @click="handleAdd">新建处方</el-button>
      </div>
    </template>
    
    <el-table :data="prescriptions" border stripe>
      <el-table-column prop="id" label="ID" width="80" />
      <el-table-column prop="prescriptionDate" label="开方日期" />
      <el-table-column prop="totalAmount" label="总金额" />
      <el-table-column prop="status" label="状态">
        <template #default="{ row }">
          <el-tag v-if="row.status === 'PENDING'" type="warning">待发药</el-tag>
          <el-tag v-else-if="row.status === 'DISPENSED'" type="success">已发药</el-tag>
          <el-tag v-else type="danger">已取消</el-tag>
        </template>
      </el-table-column>
      <el-table-column label="操作" width="250">
        <template #default="{ row }">
          <el-button size="small" @click="handleView(row)">查看</el-button>
          <el-button
            v-if="row.status === 'PENDING'"
            size="small"
            type="success"
            @click="handleDispense(row)"
          >
            发药
          </el-button>
          <el-button
            v-if="row.status === 'PENDING'"
            size="small"
            type="danger"
            @click="handleCancel(row)"
          >
            取消
          </el-button>
        </template>
      </el-table-column>
    </el-table>
    
    <el-dialog v-model="dialogVisible" title="新建处方" width="900px">
      <el-form :model="form" label-width="100px">
        <el-row :gutter="20">
          <el-col :span="12">
            <el-form-item label="病历">
              <el-select v-model="form.medicalRecordId" filterable @change="handleRecordChange">
                <el-option
                  v-for="record in medicalRecords"
                  :key="record.id"
                  :label="`${record.patient?.realName} - ${record.visitDate}`"
                  :value="record.id"
                />
              </el-select>
            </el-form-item>
          </el-col>
          <el-col :span="12">
            <el-form-item label="患者">
              <el-input v-model="form.patientName" disabled />
            </el-form-item>
          </el-col>
        </el-row>
        <el-form-item label="处方明细">
          <el-button type="primary" size="small" @click="handleAddItem">添加药品</el-button>
          <el-table :data="form.items" border style="margin-top: 10px">
            <el-table-column prop="medicine.name" label="药品名称" />
            <el-table-column prop="quantity" label="数量" width="100">
              <template #default="{ row, $index }">
                <el-input-number
                  v-model="row.quantity"
                  :min="1"
                  size="small"
                  @change="calculateTotal"
                />
              </template>
            </el-table-column>
            <el-table-column prop="dosage" label="用法用量" width="150">
              <template #default="{ row }">
                <el-input v-model="row.dosage" size="small" />
              </template>
            </el-table-column>
            <el-table-column prop="frequency" label="频次" width="120">
              <template #default="{ row }">
                <el-input v-model="row.frequency" size="small" />
              </template>
            </el-table-column>
            <el-table-column prop="totalPrice" label="小计" />
            <el-table-column label="操作" width="100">
              <template #default="{ $index }">
                <el-button size="small" type="danger" @click="handleRemoveItem($index)">删除</el-button>
              </template>
            </el-table-column>
          </el-table>
        </el-form-item>
        <el-form-item label="总金额">
          <span style="font-size: 18px; font-weight: bold; color: #409EFF">
            ¥{{ totalAmount.toFixed(2) }}
          </span>
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
    
    <el-dialog v-model="itemDialogVisible" title="选择药品" width="600px">
      <el-select v-model="selectedMedicine" filterable style="width: 100%">
        <el-option
          v-for="medicine in medicines"
          :key="medicine.id"
          :label="`${medicine.name} - ${medicine.specification} - ¥${medicine.price}`"
          :value="medicine.id"
        />
      </el-select>
      <template #footer>
        <el-button @click="itemDialogVisible = false">取消</el-button>
        <el-button type="primary" @click="handleAddItemSubmit">确定</el-button>
      </template>
    </el-dialog>
    
    <el-dialog v-model="viewVisible" title="处方详情" width="800px">
      <el-descriptions :column="2" border>
        <el-descriptions-item label="开方日期">{{ currentPrescription.prescriptionDate }}</el-descriptions-item>
        <el-descriptions-item label="总金额">¥{{ currentPrescription.totalAmount }}</el-descriptions-item>
        <el-descriptions-item label="状态">
          <el-tag v-if="currentPrescription.status === 'PENDING'" type="warning">待发药</el-tag>
          <el-tag v-else-if="currentPrescription.status === 'DISPENSED'" type="success">已发药</el-tag>
          <el-tag v-else type="danger">已取消</el-tag>
        </el-descriptions-item>
        <el-descriptions-item label="备注">{{ currentPrescription.remarks }}</el-descriptions-item>
      </el-descriptions>
      <el-table :data="currentPrescription.items" border style="margin-top: 20px">
        <el-table-column prop="medicine.name" label="药品名称" />
        <el-table-column prop="quantity" label="数量" />
        <el-table-column prop="dosage" label="用法用量" />
        <el-table-column prop="frequency" label="频次" />
        <el-table-column prop="unitPrice" label="单价" />
        <el-table-column prop="totalPrice" label="小计" />
      </el-table>
    </el-dialog>
  </el-card>
</template>

<script setup>
import { ref, onMounted, computed } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { useUserStore } from '../../stores/user'
import api from '../../utils/api'

const prescriptions = ref([])
const medicalRecords = ref([])
const medicines = ref([])
const dialogVisible = ref(false)
const itemDialogVisible = ref(false)
const viewVisible = ref(false)
const form = ref({ items: [] })
const selectedMedicine = ref(null)
const currentPrescription = ref({})
const userStore = useUserStore()

const totalAmount = computed(() => {
  return form.value.items.reduce((sum, item) => {
    return sum + (item.totalPrice || 0)
  }, 0)
})

const loadPrescriptions = async () => {
  try {
    // 根据角色加载不同的处方列表
    if (userStore.isPatient()) {
      const user = JSON.parse(localStorage.getItem('user') || '{}')
      const res = await api.get(`/prescription/patient/${user.id}`)
      prescriptions.value = res.data || []
    } else {
      // 管理员和医生可以查看所有处方，这里简化处理
      const res = await api.get('/prescription/list')
      prescriptions.value = res.data || []
    }
  } catch (error) {
    // 如果列表接口不存在，忽略错误
    prescriptions.value = []
  }
}

const loadMedicalRecords = async () => {
  try {
    let res
    if (userStore.isDoctor()) {
      const user = JSON.parse(localStorage.getItem('user') || '{}')
      res = await api.get(`/medical-record/doctor/${user.id}`)
    } else {
      res = await api.get('/medical-record/list')
    }
    medicalRecords.value = res.data || []
  } catch (error) {
    console.error('加载病历列表失败', error)
  }
}

const loadMedicines = async () => {
  try {
    const res = await api.get('/medicine/list')
    medicines.value = res.data || []
  } catch (error) {
    console.error('加载药品列表失败', error)
  }
}

const handleAdd = async () => {
  await loadMedicalRecords()
  await loadMedicines()
  const user = JSON.parse(localStorage.getItem('user') || '{}')
  form.value = {
    doctorId: user.id,
    items: []
  }
  dialogVisible.value = true
}

const handleRecordChange = async (recordId) => {
  const record = medicalRecords.value.find(r => r.id === recordId)
  if (record) {
    form.value.patientId = record.patientId
    form.value.patientName = record.patient?.realName
  }
}

const handleAddItem = () => {
  selectedMedicine.value = null
  itemDialogVisible.value = true
}

const handleAddItemSubmit = async () => {
  if (!selectedMedicine.value) {
    ElMessage.warning('请选择药品')
    return
  }
  const medicine = medicines.value.find(m => m.id === selectedMedicine.value)
  if (medicine) {
    const item = {
      medicineId: medicine.id,
      medicine: medicine,
      quantity: 1,
      dosage: '',
      frequency: '',
      unitPrice: medicine.price,
      totalPrice: medicine.price
    }
    form.value.items.push(item)
    itemDialogVisible.value = false
    calculateTotal()
  }
}

const handleRemoveItem = (index) => {
  form.value.items.splice(index, 1)
  calculateTotal()
}

const calculateTotal = () => {
  form.value.items.forEach(item => {
    item.totalPrice = item.unitPrice * item.quantity
  })
}

const handleSubmit = async () => {
  if (!form.value.medicalRecordId) {
    ElMessage.warning('请选择病历')
    return
  }
  if (form.value.items.length === 0) {
    ElMessage.warning('请至少添加一种药品')
    return
  }
  try {
    const data = {
      medicalRecordId: form.value.medicalRecordId,
      patientId: form.value.patientId,
      doctorId: form.value.doctorId,
      remarks: form.value.remarks,
      items: form.value.items.map(item => ({
        medicineId: item.medicineId,
        quantity: item.quantity,
        dosage: item.dosage,
        frequency: item.frequency
      }))
    }
    await api.post('/prescription', data)
    ElMessage.success('创建成功')
    dialogVisible.value = false
    loadPrescriptions()
  } catch (error) {
    ElMessage.error(error.message || '操作失败')
  }
}

const handleView = async (row) => {
  try {
    const res = await api.get(`/prescription/${row.id}`)
    currentPrescription.value = res.data || row
    viewVisible.value = true
  } catch (error) {
    ElMessage.error('加载处方详情失败')
  }
}

const handleDispense = async (row) => {
  try {
    await api.put(`/prescription/${row.id}/status`, { status: 'DISPENSED' })
    ElMessage.success('发药成功')
    loadPrescriptions()
  } catch (error) {
    ElMessage.error('操作失败')
  }
}

const handleCancel = async (row) => {
  try {
    await ElMessageBox.confirm('确定要取消该处方吗？', '提示', {
      type: 'warning'
    })
    await api.put(`/prescription/${row.id}/status`, { status: 'CANCELLED' })
    ElMessage.success('取消成功')
    loadPrescriptions()
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('操作失败')
    }
  }
}

onMounted(() => {
  loadPrescriptions()
})
</script>

<style scoped>
.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}
</style>

