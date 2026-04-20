import { defineStore } from 'pinia'
import { ref } from 'vue'
import api from '../utils/api'

export const useUserStore = defineStore('user', () => {
  const user = ref(null)
  const token = ref(localStorage.getItem('token') || null)

  const login = async (username, password) => {
    const response = await api.post('/auth/login', { username, password })
    token.value = response.data.token
    user.value = response.data.user
    localStorage.setItem('token', token.value)
    localStorage.setItem('user', JSON.stringify(user.value))
    return response
  }

  const logout = () => {
    token.value = null
    user.value = null
    localStorage.removeItem('token')
    localStorage.removeItem('user')
  }

  const isAdmin = () => {
    return user.value?.role === 'SUPER_ADMIN'
  }

  const isDoctor = () => {
    return user.value?.role === 'DOCTOR'
  }

  const isPatient = () => {
    return user.value?.role === 'PATIENT'
  }

  return {
    user,
    token,
    login,
    logout,
    isAdmin,
    isDoctor,
    isPatient
  }
})

