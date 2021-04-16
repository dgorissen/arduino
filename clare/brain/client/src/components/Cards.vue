<template>
  <b-container>
    <b-row>
      <b-col l="4">
        <b-card
          title="Tracks"
          img-src="../assets/img/tracks.png"
          img-alt="Image"
          img-top
          tag="article"
          style="max-width: 20rem"
          class="mb-2"
        >
          <b-card-text>Track controller</b-card-text>
          <b-button
            :disabled="connected"
            v-on:click="connect()"
            variant="primary"
            >Connect</b-button
          >
          <p />
          <b-button
            :disabled="!connected"
            :pressed.sync="headlights"
            variant="primary"
            >Turn headlights {{ headlightActionStr() }}</b-button
          >
          <p />
          Current state: {{ state }}
        </b-card>
      </b-col>
    </b-row>
  </b-container>
</template>

<script>
import axios from "axios";
var api = process.env.VUE_APP_API_LOCATION;

export default {
  data() {
    return {
      headlights: false,
      connected: false,
      state: "",
    };
  },
  watch: {
    headlights: function (val) {
      axios
        .get(api + "/tracks/headlights/" + (val ? "on" : "off"))
        .then((response) => {
          console.log(response);
        })
        .catch((err) => {
          console.log(err);
        });
    },
  },
  methods: {
    connect: function () {
      axios
        .get(api + "/tracks/connect")
        .then((res) => {
          this.connected = res.data.connected;
        })
        .catch((err) => {
          console.log(err);
        });
    },
    headlightActionStr: function () {
      return this.headlights ? "off" : "on";
    },
  },
  mounted() {
    axios
      .get(api + "/tracks/headlights")
      .then((response) => {
        this.headlights = response.data.value == "True" ? true : false;
      })
      .catch((err) => {
        console.log(err);
      });

    // Temporarily get state
    // TODO: replace by event source
    setInterval(() => {
      axios
        .get(api + "/tracks/state")
        .then((response) => {
          this.state = response.data;
        })
        .catch((err) => {
          console.log(err);
          this.state = 'Failed to get state';
        });
    }, 2000);
  },
};
</script>