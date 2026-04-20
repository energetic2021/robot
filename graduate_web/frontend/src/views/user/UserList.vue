<template>
  <el-card>
    <template #header>
      <div class="card-header">
        <span>用户管理</span>
        <el-button type="primary" @click="handleAdd">新增用户</el-button>
      </div>
    </template>
    
    <el-table :data="users" border stripe>
      <el-table-column prop="id" label="ID" width="80" />
      <el-table-column prop="username" label="用户名" />
      <el-table-column prop="realName" label="姓名" />
      <el-table-column prop="phone" label="电话" />
      <el-table-column prop="email" label="邮箱" />
      <el-table-column prop="role" label="角色">
        <template #default="{ row }">
          <el-tag v-if="row.role === 'SUPER_ADMIN'" type="danger">超级管理员</el-tag>
          <el-tag v-else-if="row.role === 'DOCTOR'" type="success">医生</el-tag>
          <el-tag v-else type="info">患者</el-tag>
        </template>
      </el-table-column>
      <el-table-column prop="status" label="状态">
        <template #default="{ row }">
          <el-tag :type="row.status === 1 ? 'success' : 'danger'">
            {{ row.status === 1 ? '启用' : '禁用' }}
          </el-tag>
        </template>
      </el-table-column>
      <el-table-column label="操作" width="200">
        <template #default="{ row }">
          <el-button size="small" @click="handleEdit(row)">编辑</el-button>
          <el-button size="small" type="danger" @click="handleDelete(row)">删除</el-button>
        </template>
      </el-table-column>
    </el-table>
    
    <el-dialog v-model="dialogVisible" :title="dialogTitle" width="500px">
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
        <el-form-item label="角色">
          <el-select v-model="form.role">
            <el-option label="超级管理员" value="SUPER_ADMIN" />
            <el-option label="医生" value="DOCTOR" />
            <el-option label="患者" value="PATIENT" />
          </el-select>
        </el-form-item>
        <template v-if="form.role === 'PATIENT'">
          <el-form-item label="患者类型">
            <el-radio-group v-model="form.patientType">
              <el-radio label="OUTPATIENT">门诊</el-radio>
              <el-radio label="INPATIENT">住院</el-radio>
            </el-radio-group>
          </el-form-item>
          <el-form-item label="房号" v-if="form.patientType === 'INPATIENT'">
            <el-input v-model="form.roomNumber" placeholder="请输入房号" />
          </el-form-item>
        </template>
        <el-form-item label="状态">
          <el-switch v-model="form.status" :active-value="1" :inactive-value="0" />
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

const users = ref([])
const dialogVisible = ref(false)
const dialogTitle = ref('新增用户')
const form = ref({})

const loadUsers = async () => {
  try {
    const res = await api.get('/user/list')
    users.value = res.data || []
  } catch (error) {
    ElMessage.error('加载用户列表失败')
  }
}

const handleAdd = () => {
  form.value = { role: 'PATIENT', status: 1, patientType: 'OUTPATIENT' }
  dialogTitle.value = '新增用户'
  dialogVisible.value = true
}

const handleEdit = (row) => {
  form.value = { ...row }
  dialogTitle.value = '编辑用户'
  dialogVisible.value = true
}

const handleSubmit = async () => {
  try {
    if (form.value.id) {
      await api.put(`/user/${form.value.id}`, form.value)
      ElMessage.success('更新成功')
    } else {
      await api.post('/user', form.value)
      ElMessage.success('创建成功')
    }
    dialogVisible.value = false
    loadUsers()
  } catch (error) {
    ElMessage.error(error.message || '操作失败')
  }
}

const handleDelete = async (row) => {
  try {
    await ElMessageBox.confirm('确定要删除该用户吗？', '提示', {
      type: 'warning'
    })
    await api.delete(`/user/${row.id}`)
    ElMessage.success('删除成功')
    loadUsers()
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('删除失败')
    }
  }
}

onMounted(() => {
  loadUsers()
})
</script>

<style scoped>
.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}
</style>

