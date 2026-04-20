<template>
  <el-card>
    <template #header>
      <div class="card-header">
        <span>医生管理</span>
        <div class="header-actions">
          <el-input
            v-model="searchName"
            placeholder="按姓名搜索（支持模糊）"
            clearable
            style="width: 220px"
            @keyup.enter="handleSearch"
          />
          <el-button :loading="searchLoading" @click="handleSearch">查询</el-button>
          <el-button :disabled="searchLoading" @click="handleReset">重置</el-button>
          <el-button type="primary" @click="handleAdd">新增医生</el-button>
        </div>
      </div>
    </template>
    
    <el-table :data="doctors" border stripe>
      <el-table-column prop="id" label="ID" width="80" />
      <el-table-column prop="user.realName" label="姓名" />
      <el-table-column prop="department" label="科室" />
      <el-table-column prop="title" label="职称" />
      <el-table-column prop="specialty" label="专业特长" />
      <el-table-column prop="user.phone" label="电话" />
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
        <el-form-item label="电话">
          <el-input v-model="form.phone" />
        </el-form-item>
        <el-form-item label="邮箱">
          <el-input v-model="form.email" />
        </el-form-item>
        <el-form-item label="科室">
          <el-input v-model="form.department" />
        </el-form-item>
        <el-form-item label="职称">
          <el-input v-model="form.title" />
        </el-form-item>
        <el-form-item label="专业特长">
          <el-input v-model="form.specialty" type="textarea" />
        </el-form-item>
        <el-form-item label="执业证书号">
          <el-input v-model="form.licenseNumber" />
        </el-form-item>
        <el-form-item label="简介">
          <el-input v-model="form.introduction" type="textarea" :rows="3" />
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

const doctors = ref([])
const searchName = ref('')
const searchLoading = ref(false)
const dialogVisible = ref(false)
const dialogTitle = ref('新增医生')
const form = ref({})

const loadDoctors = async () => {
  try {
    const res = await api.get('/doctor/list')
    doctors.value = res.data || []
  } catch (error) {
    ElMessage.error('加载医生列表失败')
  }
}

const handleSearch = async () => {
  const name = (searchName.value || '').trim()
  if (!name) {
    loadDoctors()
    return
  }
  searchLoading.value = true
  try {
    const res = await api.get('/doctor/search', { params: { name } })
    doctors.value = res.data || []
  } catch (error) {
    ElMessage.error('按姓名查询失败')
  } finally {
    searchLoading.value = false
  }
}

const handleReset = () => {
  searchName.value = ''
  loadDoctors()
}

const handleAdd = () => {
  form.value = {}
  dialogTitle.value = '新增医生'
  dialogVisible.value = true
}

const handleEdit = (row) => {
  form.value = {
    id: row.id,
    username: row.user?.username || '',
    realName: row.user?.realName || '',
    phone: row.user?.phone || '',
    email: row.user?.email || '',
    department: row.department,
    title: row.title,
    specialty: row.specialty,
    licenseNumber: row.licenseNumber,
    introduction: row.introduction
  }
  dialogTitle.value = '编辑医生'
  dialogVisible.value = true
}

const handleSubmit = async () => {
  try {
    if (form.value.id) {
      await api.put(`/doctor/${form.value.id}`, form.value)
      ElMessage.success('更新成功')
    } else {
      await api.post('/doctor', form.value)
      ElMessage.success('创建成功')
    }
    dialogVisible.value = false
    loadDoctors()
  } catch (error) {
    ElMessage.error(error.message || '操作失败')
  }
}

const handleDelete = async (row) => {
  try {
    await ElMessageBox.confirm('确定要删除该医生吗？', '提示', {
      type: 'warning'
    })
    await api.delete(`/doctor/${row.id}`)
    ElMessage.success('删除成功')
    loadDoctors()
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('删除失败')
    }
  }
}

onMounted(() => {
  loadDoctors()
})
</script>

<style scoped>
.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.header-actions {
  display: flex;
  align-items: center;
  gap: 10px;
}
</style>

