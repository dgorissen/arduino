<template>
  <div id="app">
    <Navbar />
    <Body/>
  </div>
</template>

<script>
  import Body from './components/Body.vue'
  import Navbar from './components/Navbar.vue';  
  export default {
    name: 'navbar',
    components: {
      Navbar,
      Body
    }
  }
</script>