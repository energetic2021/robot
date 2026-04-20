<template>
  <el-card>
    <template #header>
      <div class="card-header">
        <span>药品管理</span>
        <el-button type="primary" @click="handleAdd">新增药品</el-button>
      </div>
    </template>
    
    <el-input
      v-model="searchKeyword"
      placeholder="搜索药品名称"
      style="width: 300px; margin-bottom: 20px"
      @input="handleSearch"
    />
    
    <el-table :data="medicines" border stripe>
      <el-table-column prop="id" label="ID" width="80" />
      <el-table-column prop="name" label="药品名称" />
      <el-table-column prop="code" label="药品编码" />
      <el-table-column prop="specification" label="规格" />
      <el-table-column prop="unit" label="单位" />
      <el-table-column prop="price" label="价格" />
      <el-table-column prop="stock" label="库存" />
      <el-table-column prop="manufacturer" label="生产厂家" />
      <el-table-column prop="status" label="状态">
        <template #default="{ row }">
          <el-tag :type="row.status === 1 ? 'success' : 'danger'">
            {{ row.status === 1 ? '可用' : '停用' }}
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
    
    <el-dialog v-model="dialogVisible" :title="dialogTitle" width="600px">
      <el-form :model="form" label-width="100px">
        <el-form-item label="药品名称">
          <el-input v-model="form.name" />
        </el-form-item>
        <el-form-item label="药品编码">
          <el-input v-model="form.code" />
        </el-form-item>
        <el-form-item label="规格">
          <el-input v-model="form.specification" />
        </el-form-item>
        <el-form-item label="单位">
          <el-input v-model="form.unit" />
        </el-form-item>
        <el-form-item label="价格">
          <el-input-number v-model="form.price" :precision="2" :min="0" />
        </el-form-item>
        <el-form-item label="库存">
          <el-input-number v-model="form.stock" :min="0" />
        </el-form-item>
        <el-form-item label="生产厂家">
          <el-input v-model="form.manufacturer" />
        </el-form-item>
        <el-form-item label="说明">
          <el-input v-model="form.description" type="textarea" :rows="3" />
        </el-form-item>
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

const medicines = ref([])
const searchKeyword = ref('')
const dialogVisible = ref(false)
const dialogTitle = ref('新增药品')
const form = ref({})

const loadMedicines = async () => {
  try {
    const res = await api.get('/medicine/list')
    medicines.value = res.data || []
  } catch (error) {
    ElMessage.error('加载药品列表失败')
  }
}

const handleSearch = async () => {
  if (!searchKeyword.value) {
    loadMedicines()
    return
  }
  try {
    const res = await api.get(`/medicine/search?keyword=${searchKeyword.value}`)
    medicines.value = res.data || []
  } catch (error) {
    ElMessage.error('搜索失败')
  }
}

const handleAdd = () => {
  form.value = { status: 1, price: 0, stock: 0 }
  dialogTitle.value = '新增药品'
  dialogVisible.value = true
}

const handleEdit = (row) => {
  form.value = { ...row }
  dialogTitle.value = '编辑药品'
  dialogVisible.value = true
}

const handleSubmit = async () => {
  try {
    if (form.value.id) {
      await api.put(`/medicine/${form.value.id}`, form.value)
      ElMessage.success('更新成功')
    } else {
      await api.post('/medicine', form.value)
      ElMessage.success('创建成功')
    }
    dialogVisible.value = false
    loadMedicines()
  } catch (error) {
    ElMessage.error(error.message || '操作失败')
  }
}

const handleDelete = async (row) => {
  try {
    await ElMessageBox.confirm('确定要删除该药品吗？', '提示', {
      type: 'warning'
    })
    await api.delete(`/medicine/${row.id}`)
    ElMessage.success('删除成功')
    loadMedicines()
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('删除失败')
    }
  }
}

onMounted(() => {
  loadMedicines()
})
</script>

<style scoped>
.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}
</style>

