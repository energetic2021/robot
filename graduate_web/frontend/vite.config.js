import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import { resolve } from 'path'

export default defineConfig({
  plugins: [vue()],
  resolve: {
    alias: {
      '@': resolve(__dirname, 'src')
    }
  },
  server: {
    port: 3000,
    proxy: {
      '/api': {
        target: 'http://localhost:8080',
        changeOrigin: true
      }
    },
    // 允许外网穿透域名访问 Vite 开发服务器（只填 host，不要带 http://）
    allowedHosts: ['j9369942.natappfree.cc']
    // allowedHosts: ['t78f6d32.natappfree.cc']
    
  }
})

