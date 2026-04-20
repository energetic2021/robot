import { createRouter, createWebHistory } from 'vue-router'
import { useUserStore } from '../stores/user'

const routes = [
  {
    path: '/login',
    name: 'Login',
    component: () => import('../views/Login.vue'),
    meta: { requiresAuth: false }
  },
  {
    path: '/',
    component: () => import('../layouts/MainLayout.vue'),
    redirect: '/dashboard',
    meta: { requiresAuth: true },
    children: [
      {
        path: 'dashboard',
        name: 'Dashboard',
        component: () => import('../views/Dashboard.vue')
      },
      {
        path: 'users',
        name: 'Users',
        component: () => import('../views/user/UserList.vue'),
        meta: { roles: ['SUPER_ADMIN'] }
      },
      {
        path: 'doctors',
        name: 'Doctors',
        component: () => import('../views/doctor/DoctorList.vue'),
        meta: { roles: ['SUPER_ADMIN'] }
      },
      {
        path: 'patients',
        name: 'Patients',
        component: () => import('../views/patient/PatientList.vue'),
        meta: { roles: ['SUPER_ADMIN', 'DOCTOR'] }
      },
      {
        path: 'medical-records',
        name: 'MedicalRecords',
        component: () => import('../views/medical-record/MedicalRecordList.vue'),
        meta: { roles: ['SUPER_ADMIN', 'DOCTOR'] }
      },
      {
        path: 'consultations',
        name: 'Consultations',
        component: () => import('../views/consultation/ConsultationList.vue'),
        meta: { roles: ['SUPER_ADMIN', 'DOCTOR'] }
      },
      {
        path: 'medicines',
        name: 'Medicines',
        component: () => import('../views/medicine/MedicineList.vue'),
        meta: { roles: ['SUPER_ADMIN', 'DOCTOR'] }
      },
      {
        path: 'prescriptions',
        name: 'Prescriptions',
        component: () => import('../views/prescription/PrescriptionList.vue'),
        meta: { roles: ['SUPER_ADMIN', 'DOCTOR'] }
      },
      {
        path: 'robot',
        name: 'Robot',
        component: () => import('../views/robot/RobotControl.vue')
      }
    ]
  }
]

const router = createRouter({
  history: createWebHistory(),
  routes
})

router.beforeEach((to, from, next) => {
  const userStore = useUserStore()
  const token = localStorage.getItem('token')
  
  if (to.meta.requiresAuth && !token) {
    next('/login')
  } else if (to.path === '/login' && token) {
    next('/')
  } else {
    if (to.meta.roles && token) {
      const user = JSON.parse(localStorage.getItem('user') || '{}')
      if (!to.meta.roles.includes(user.role)) {
        next('/dashboard')
      } else {
        next()
      }
    } else {
      next()
    }
  }
})

export default router

