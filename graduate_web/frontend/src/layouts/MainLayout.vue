<template>
  <el-container>
    <el-aside width="200px">
      <el-menu
        :default-active="activeMenu"
        router
        background-color="#304156"
        text-color="#bfcbd9"
        active-text-color="#409EFF"
      >
        <el-menu-item index="/dashboard">
          <el-icon><HomeFilled /></el-icon>
          <span>首页</span>
        </el-menu-item>
        
        <el-menu-item v-if="isAdmin" index="/users">
          <el-icon><User /></el-icon>
          <span>用户管理</span>
        </el-menu-item>
        
        <el-menu-item v-if="isAdmin" index="/doctors">
          <el-icon><UserFilled /></el-icon>
          <span>医生管理</span>
        </el-menu-item>
        
        <el-menu-item v-if="!isPatient" index="/patients">
          <el-icon><Avatar /></el-icon>
          <span>患者管理</span>
        </el-menu-item>
        
        <el-menu-item v-if="!isPatient" index="/medical-records">
          <el-icon><Document /></el-icon>
          <span>病历管理</span>
        </el-menu-item>
        
        <el-menu-item v-if="!isPatient" index="/consultations">
          <el-icon><ChatLineRound /></el-icon>
          <span>会诊管理</span>
        </el-menu-item>
        
        <el-menu-item v-if="!isPatient" index="/medicines">
          <el-icon><Box /></el-icon>
          <span>药品管理</span>
        </el-menu-item>
        
        <el-menu-item v-if="!isPatient" index="/prescriptions">
          <el-icon><Tickets /></el-icon>
          <span>处方管理</span>
        </el-menu-item>
        <el-menu-item index="/robot">
          <el-icon><Monitor /></el-icon>
          <span>机器人控制</span>
        </el-menu-item>
      </el-menu>
    </el-aside>
    
    <el-container>
      <el-header>
        <div class="header-content">
          <span class="title">医疗讯息及机器人控制系统</span>
          <div class="user-info">
            <span>{{ currentUser?.realName }}</span>
            <el-button type="text" @click="handleLogout">退出</el-button>
          </div>
        </div>
      </el-header>
      
      <el-main>
        <router-view />
      </el-main>
    </el-container>
  </el-container>
</template>

<script setup>
import { computed } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { useUserStore } from '../stores/user'
import { HomeFilled, User, UserFilled, Avatar, Document, ChatLineRound, Box, Tickets, Monitor } from '@element-plus/icons-vue'

const route = useRoute()
const router = useRouter()
const userStore = useUserStore()

const activeMenu = computed(() => route.path)
const isAdmin = computed(() => userStore.isAdmin())
const isPatient = computed(() => userStore.isPatient())
const currentUser = computed(() => {
  const userStr = localStorage.getItem('user')
  return userStr ? JSON.parse(userStr) : null
})

const handleLogout = () => {
  userStore.logout()
  router.push('/login')
}
</script>

<style scoped>
.el-container {
  height: 100vh;
}

.el-aside {
  background-color: #304156;
}

.el-header {
  background-color: #fff;
  border-bottom: 1px solid #e4e7ed;
  display: flex;
  align-items: center;
}

.header-content {
  width: 100%;
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.title {
  font-size: 20px;
  font-weight: bold;
  color: #303133;
}

.user-info {
  display: flex;
  align-items: center;
  gap: 10px;
}

.el-main {
  background-color: #f0f2f5;
  padding: 20px;
}
</style>

