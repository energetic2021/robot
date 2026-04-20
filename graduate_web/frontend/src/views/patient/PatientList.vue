<template>
  <el-card>
    <template #header>
      <div class="card-header">
        <span>患者管理</span>
        <el-button type="primary" @click="handleAdd">新增患者</el-button>
      </div>
    </template>
    
    <el-table :data="patients" border stripe>
      <el-table-column prop="id" label="ID" width="80" />
      <el-table-column prop="user.realName" label="姓名" />
      <el-table-column prop="idCard" label="身份证号" />
      <el-table-column prop="gender" label="性别">
        <template #default="{ row }">
          {{ row.gender === 'MALE' ? '男' : '女' }}
        </template>
      </el-table-column>
      <el-table-column prop="birthday" label="生日" />
      <el-table-column prop="user.phone" label="电话" />
      <el-table-column prop="patientType" label="状态">
        <template #default="{ row }">
          <el-tag :type="row.patientType === 'INPATIENT' ? 'danger' : 'success'">
            {{ row.patientType === 'INPATIENT' ? '住院' : '门诊' }}
          </el-tag>
        </template>
      </el-table-column>
      <el-table-column prop="roomNumber" label="房号" />
      <el-table-column prop="address" label="地址" />
      <el-table-column label="操作" width="200">
        <template #default="{ row }">
          <el-button size="small" @click="handleEdit(row)">编辑</el-button>
          <el-button size="small" type="danger" @click="handleDelete(row)">删除</el-button>
        </template>
      </el-table-column>
    </el-table>
    
    <el-dialog v-model="dialogVisible" :title="dialogTitle" width="600px">
      <el-form :model="form" label-width="100px">
        <el-form-item label="用户名">
          <el-input v-model="form.username" :disabled="!!form.id" />
        </el-form-item>
        <el-form-item label="密码" v-if="!form.id">
          <el-input v-model="form.password" type="password" />
        </el-form-item>
        <el-form-item label="姓名">
          <el-input v-model="form.realName" />
        </el-form-item>
        <el-form-item label="身份证号">
          <el-input v-model="form.idCard" />
        </el-form-item>
        <el-form-item label="性别">
          <el-radio-group v-model="form.gender">
            <el-radio label="MALE">男</el-radio>
            <el-radio label="FEMALE">女</el-radio>
          </el-radio-group>
        </el-form-item>
        <el-form-item label="生日">
          <el-date-picker v-model="form.birthday" type="date" value-format="YYYY-MM-DD" />
        </el-form-item>
        <el-form-item label="患者类型">
          <el-radio-group v-model="form.patientType">
            <el-radio label="OUTPATIENT">门诊</el-radio>
            <el-radio label="INPATIENT">住院</el-radio>
          </el-radio-group>
        </el-form-item>
        <el-form-item label="房号" v-if="form.patientType === 'INPATIENT'">
          <el-input v-model="form.roomNumber" placeholder="请输入房号" />
        </el-form-item>
        <el-form-item label="电话">
          <el-input v-model="form.phone" />
        </el-form-item>
        <el-form-item label="邮箱">
          <el-input v-model="form.email" />
        </el-form-item>
        <el-form-item label="地址">
          <el-input v-model="form.address" />
        </el-form-item>
        <el-form-item label="紧急联系人">
          <el-input v-model="form.emergencyContact" />
        </el-form-item>
        <el-form-item label="紧急联系电话">
          <el-input v-model="form.emergencyPhone" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="dialogVisible = false">取消</el-button>
        <el-button type="primary" @click="handleSubmit">确定</el-button>
      </template>
    </el-dialog>
  </el-card>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import api from '../../utils/api'

const patients = ref([])
const dialogVisible = ref(false)
const dialogTitle = ref('新增患者')
const form = ref({})

const loadPatients = async () => {
  try {
    const res = await api.get('/patient/list')
    patients.value = res.data || []
  } catch (error) {
    ElMessage.error('加载患者列表失败')
  }
}

const handleAdd = () => {
  form.value = { gender: 'MALE', patientType: 'OUTPATIENT' }
  dialogTitle.value = '新增患者'
  dialogVisible.value = true
}

const handleEdit = (row) => {
  form.value = {
    id: row.id,
    username: row.user?.username || '',
    realName: row.user?.realName || '',
    phone: row.user?.phone || '',
    email: row.user?.email || '',
    idCard: row.idCard,
    gender: row.gender,
    birthday: row.birthday,
    address: row.address,
    emergencyContact: row.emergencyContact,
    emergencyPhone: row.emergencyPhone,
    patientType: row.patientType || 'OUTPATIENT',
    roomNumber: row.roomNumber
  }
  dialogTitle.value = '编辑患者'
  dialogVisible.value = true
}

const handleSubmit = async () => {
  try {
    if (form.value.id) {
      await api.put(`/patient/${form.value.id}`, form.value)
      ElMessage.success('更新成功')
    } else {
      await api.post('/patient', form.value)
      ElMessage.success('创建成功')
    }
    dialogVisible.value = false
    loadPatients()
  } catch (error) {
    ElMessage.error(error.message || '操作失败')
  }
}

const handleDelete = async (row) => {
  try {
    await ElMessageBox.confirm('确定要删除该患者吗？', '提示', {
      type: 'warning'
    })
    await api.delete(`/patient/${row.id}`)
    ElMessage.success('删除成功')
    loadPatients()
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('删除失败')
    }
  }
}

onMounted(() => {
  loadPatients()
})
</script>

<style scoped>
.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}
</style>

