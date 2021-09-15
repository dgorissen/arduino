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
          <!-- Current state: {{ track_state }} -->
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
          <!-- Current state: {{ middle_state }} -->
          <b-table stacked :items="[middle_state]"></b-table>
        </b-card>
      </b-col>

      <b-col l="4">
        <b-card
            title="Camera"
            img-src="../assets/img/selfie.png"
            img-alt="Image"
            img-top
            tag="article"
            style="max-width: 20rem"
            class="mb-2"
          >
          <!--b-card-text>Camera actions</b-card-text>
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
          <b-img v-bind:src="photo_src" rounded fluid alt=""></b-img-->

          <p />
          Video feed:
          <p />
          <b-img id="vid-feed" src rounded fluid alt=""></b-img>
        </b-card>
      </b-col>
    </b-row>
    <b-row>
      <b-col l="4">
        <b-card
            title="Voice"
            img-src="../assets/img/voice.png"
            img-alt="Image"
            img-top
            tag="article"
            style="max-width: 20rem"
            class="mb-2"
          >
          <b-card-text>Voice actions</b-card-text>
          <p />
          <b-button
            :disabled="!voice_ready"
            v-on:click="speak()"
            variant="primary"
            >Speak to me</b-button
          >
          <b-form-input v-model="tts_input" placeholder="Hello. I am Clare and I do not like aubergines"></b-form-input>
          <p />
          What I have heard so far:
          <p />
          <b-form-textarea
            id="textarea"
            v-model="stt_output"
            placeholder="No data yet..."
            rows="3"
            max-rows="6"
          ></b-form-textarea>
        </b-card>
      </b-col>
    </b-row>
  </b-container>
</template>

<script>
import axios from "axios";
var api = process.env.VUE_APP_API_LOCATION;

// EventSources
var tracksStateStream = null;
var middleStateStream = null;
var sttStream = null;
//var headStateStream = null;

function setupEventSource(src, name, handler) {
  src.addEventListener('open', (e) => {
    console.log("EventSource " + name + " opened");
    console.log(e);
  }, false);

  src.addEventListener('error', (e) => {
    console.log("Error in EventSource " + name);
    console.log(e);
    if (e.readyState == EventSource.CLOSED) {
      console.log("EventSource " + name + " closed");
    }
  }, false);

  src.addEventListener(name, (event) => {
    handler(JSON.parse(event.data));
  });

  src.addEventListener("message", (event) => {
    console.log("Got generic message: " + event);
  });
}

function closeEventSources() {
  if (tracksStateStream != null) tracksStateStream.close();
  if (middleStateStream != null) middleStateStream.close();
  if (sttStream != null) sttStream.close();
  //if (headStateStream != null) headStateStream.close();
}

export default {
  data() {
    return {
      headlights: false,
      connected: null,
      track_state: {},
      middle_state: {},
      head_state: {},
      photo_ready: true,
      photo_src: "",
      tts_input: "",
      stt_output: "",
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
    connected: function (val) {
      if(val) {
        this.connectToStreams();
        document.getElementById('vid-feed')
         .setAttribute('src', api + '/head/facefeed');
      } else {
        this.closeStreams();
        document.getElementById('vid-feed')
         .setAttribute('src', '');
      }
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
    speak: function () {
      this.voice_ready = false;
      const params = new URLSearchParams([['tts', this.tts_input]]);
      axios
        .get(api + "/voice/speak", {params})
        .then((res) => {
          this.voice_ready = true;
        })
        .catch((err) => {
          console.log(err);
        });
    },
    connectToStreams: function(){
      tracksStateStream = new EventSource(api + "/tracks/stream");
      setupEventSource(tracksStateStream, "tracks", (data) => this.track_state = data);

      middleStateStream = new EventSource(api + "/middle/stream");
      setupEventSource(middleStateStream, "middle", (data) => this.middle_state = data);

      sttStream = new EventSource(api + "/voice/stream");
      setupEventSource(sttStream, "stt", (data) => this.tts_output.concat(data + "\n"));

      // headStateStream = new EventSource(api + "/head/stream");
      // setupEventSource(headStateStream, "head", (data) => this.head_state = data);
    },
    closeStreams: function() {
      closeEventSources();
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
  },
};
</script>