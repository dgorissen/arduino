<template>
  <div id="app">
    <Navbar />
    <Body/>
  </div>
</template>

<style>
  @import "./assets/styles/main.css";
</style>

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
