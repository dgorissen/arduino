<template>
  <b-container>
    <b-row>
      <b-button
        :disabled="connected"
        v-on:click="connect()"
        variant="primary"
        >Connect</b-button
      >
    </b-row>
    <p />

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
          <b-card-text>Track actions</b-card-text>
          <p />
          <b-button
            :disabled="!connected"
            :pressed.sync="headlights"
            variant="primary"
            >Turn headlights {{ headlightActionStr() }}</b-button
          >
          <p />
          Current state:
          <b-table stacked :items="[track_state]"></b-table>
        </b-card>
      </b-col>

      <b-col l="4">
        <b-card
          title="Middle"
          img-src="../assets/img/middle.png"
          img-alt="Image"
          img-top
          tag="article"
          style="max-width: 20rem"
          class="mb-2"
        >
          <b-card-text>Middle actions</b-card-text>
          <p />
          Current state:
          <b-table stacked :items="[middle_state]"></b-table>
        </b-card>
      </b-col>

      <b-col l="4">
        <b-card
            title="Photo"
            img-src="../assets/img/selfie.png"
            img-alt="Image"
            img-top
            tag="article"
            style="max-width: 20rem"
            class="mb-2"
          >
          <b-card-text>Photo actions</b-card-text>
          <p />
          <b-button
            :disabled="!photo_ready"
            v-on:click="takePhoto()"
            variant="primary"
            >Take Photo</b-button
          >
          <p />
          Last Photo:
          <p />
          <b-img v-bind:src="photo_src" rounded fluid alt=""></b-img>
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
      track_state: "",
      middle_state: "",
      photo_ready: true,
      photo_src: "",
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
        .get(api + "/connect")
        .then((res) => {
          this.connected = true;
          console.log("Connected: " + res.data);
        })
        .catch((err) => {
          console.log(err);
        });
    },
    headlightActionStr: function () {
      return this.headlights ? "off" : "on";
    },
    takePhoto: function () {
      this.photo_ready = false;
      axios
        .get(api + "/make_photo")
        .then((res) => {
          this.photo_ready = true;
          this.photo_src = api + "/photo/" + res.data.name;
        })
        .catch((err) => {
          console.log(err);
        });
    },

  },
  mounted() {
    axios
      .get(api + "/")
      .then((response) => {
        this.connected = response.data.connected;
      })
      .catch((err) => {
        console.log(err);
      });

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
        if(this.connected) {
          axios
          .get(api + "/tracks/state")
          .then((response) => {
            this.track_state = response.data;
          })
          .catch((err) => {
            console.log(err);
            this.track_state = 'Failed to get state';
          });

          axios
          .get(api + "/middle/state")
          .then((response) => {
            this.middle_state = response.data;
          })
          .catch((err) => {
            console.log(err);
            this.middle_state = 'Failed to get state';
          });
        }
    }, 2000);
  },
};
</script>