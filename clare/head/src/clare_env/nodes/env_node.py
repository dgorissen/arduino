#!/usr/bin/env python
import time
import rospy
import adafruit_bme680
from adafruit_extended_bus import ExtendedI2C as I2C
from clare_env.msg import BME680Message


class EnvMonitor(object):
    def __init__(self):
        super(EnvMonitor, self).__init__()
        
        rospy.init_node("clare_env", anonymous=False, disable_signals=False)
        self._env_pub = rospy.Publisher("clare/env", BME680Message, queue_size=10)
        
        i2c_bus = rospy.get_param("~i2c_bus", 3)
        i2c = I2C(i2c_bus)
        self._env_sensor = adafruit_bme680.Adafruit_BME680_I2C(i2c)
        self._env_sensor.sea_level_pressure = 1013.25

    def run(self):
        rospy.logdebug('Env node ready and publishing')
        rate = rospy.Rate(1)
        m = BME680Message()

        while not rospy.is_shutdown():
            m.temp = self._env_sensor.temperature
            m.gas = self._env_sensor.gas
            m = humidity = self._env_sensor.relative_humidity
            m.pressure = self._env_sensor.pressure
            m.altitude = self._env_sensor.altitude

            self._env_pub.publish(m)
            rate.sleep()

if __name__ == "__main__":
  
    n = EnvMonitor()
    try:
        n.run()
    except rospy.ROSInterruptException:
        pass
