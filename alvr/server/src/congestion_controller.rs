use std::{
    collections::{HashMap, VecDeque},
    time::{Duration, Instant, self}, f64::{NAN, INFINITY}, 
};
//use crate::gcc_config;
use std::error::Error;
use std::fs::OpenOptions;
use std::io::prelude::*;
use csv::Writer;
use chrono::{Utc, TimeZone};

const WINDOW_SIZE : usize = 5;

#[derive(PartialEq)]
pub enum BandwidthUsage {
    kBwNormal = 0,
    kBwUnderusing = 1,
    kBwOverusing = 2,
    kLast,
}

pub struct PacketTiming{
    pub arrival_time_ms:f64,
    pub smoothed_delay_ms:f64,
    pub raw_delay_ms:f64,
}
impl PacketTiming {
    fn new(arrival_time_ms: f64, smoothed_delay_ms: f64, raw_delay_ms: f64) -> Self {
        Self {
            arrival_time_ms,
            smoothed_delay_ms,
            raw_delay_ms,
        }
    }
}

    // EyeNexus-Network Monitoring: Compute Queuing Delay Gradient (Eq. 6) to detect congestion
    // Calculates the slope of the delay trend over a window of packets.
    pub fn LinearFitSlope(packets:& VecDeque<PacketTiming>)->Option<f64> {
        if packets.len()>2 {
                  // Compute the "center of mass".
              let mut sum_x = 0.0;
              let mut sum_y = 0.0;
              for packet in packets {
                  sum_x += packet.arrival_time_ms;
                  sum_y += packet.smoothed_delay_ms;
              }
              let x_avg = sum_x / packets.len() as f64;
              let y_avg = sum_y / packets.len() as f64;
              // Compute the slope k = \sum (x_i-x_avg)(y_i-y_avg) / \sum (x_i-x_avg)^2
              let mut numerator = 0.0;
              let mut denominator = 0.0;
              for packet in packets {
                  let x = packet.arrival_time_ms;
                  let y = packet.smoothed_delay_ms;
                  numerator += (x - x_avg) * (y - y_avg);
                  denominator += (x - x_avg) * (x - x_avg);
              }
              if denominator == 0.0{
                  return Option::None;
              }
                  
              return Some(numerator / denominator);
        }else{
          return Option::None;
        }
      }
  pub struct TrendlineEstimator{
      //TrendlineEstimatorSettings settings_;
      pub smoothing_coef_: f64,
      pub threshold_gain_: f64,
      // Used by the existing threshold.
      pub num_of_deltas_: i64,
      // Keep the arrival times small by using the change from the first packet.
      pub first_arrival_time_ms_: i64,
      // Exponential backoff filtering.
      pub accumulated_delay_: f64,
      pub smoothed_delay_: f64,
      // Linear least squares regression.
      
      pub k_up_:f64,
      pub k_down_:f64,
      pub overusing_time_threshold_: f64,
      pub threshold_: f64,
      pub prev_modified_trend_: f64,
      pub last_update_ms_: i64,
      pub prev_trend_:f64,
      pub time_over_using_:f64,
      pub overuse_counter_: i64,
      pub hypothesis_: BandwidthUsage,
      pub hypothesis_predicted_: BandwidthUsage,
      //NetworkStatePredictor* network_state_predictor_;
      pub delay_hist_:VecDeque<PacketTiming>,
      pub current_trend_for_testing:f64,
      pub current_threshold_for_testing:f64,
  }
  impl TrendlineEstimator{
      pub fn new() -> Self {
          Self {
              
              smoothing_coef_: 0.9,
              threshold_gain_: 4.0,
              // Used by the existing threshold.
              num_of_deltas_: 0,
              // Keep the arrival times small by using the change from the first packet.
              first_arrival_time_ms_: -1,
              // Exponential backoff filtering.
              accumulated_delay_: 0.0,
              smoothed_delay_: 0.0,
              // Linear least squares regression.
              k_up_:0.0087,
              k_down_:0.039,
              overusing_time_threshold_: 10.0,
              threshold_: 6.0,//12.5
              prev_modified_trend_: NAN,
              last_update_ms_: -1,
              prev_trend_:0.0,
              time_over_using_:-1.0,
              overuse_counter_: 0,
              hypothesis_: BandwidthUsage::kBwNormal,
              hypothesis_predicted_: BandwidthUsage::kBwNormal,
              //NetworkStatePredictor* network_state_predictor_;
              delay_hist_:VecDeque::new(),
              current_threshold_for_testing:0.0,
              current_trend_for_testing:0.0,
          }
      }
      pub fn UpdateThreshold(&mut self,modified_trend:f64,
          now_ms: i64) {
          if self.last_update_ms_ == -1{
              self.last_update_ms_ = now_ms;
          }
          
          if modified_trend.abs() > self.threshold_ + 15.0 {
          // Avoid adapting the threshold to big latency spikes, caused e.g.,
          // by a sudden capacity drop.
          self.last_update_ms_ = now_ms;
          return;
          }
          let k = if modified_trend.abs() < self.threshold_ {
              self.k_down_
          } else {
              self.k_up_
          };
          
          let kMaxTimeDeltaMs = 100;
          let time_delta_ms = std::cmp::min(now_ms - self.last_update_ms_, kMaxTimeDeltaMs);
          self.threshold_ += k * (modified_trend.abs() - self.threshold_) * time_delta_ms as f64;
          if self.threshold_>600.0 as f64{
              self.threshold_=600.0;
          }else if self.threshold_<6.0 as f64{
              self.threshold_=6.0;
          }
          self.last_update_ms_ = now_ms;
      }
      pub fn Detect( &mut self,trend: f64,ts_delta: f64, now_ms: i64) {
          if self.num_of_deltas_ < 2 {  
            self.hypothesis_ = BandwidthUsage::kBwNormal;
            return;
          }
          let modified_trend =
              std::cmp::min(self.num_of_deltas_, 60) as f64 * trend * self.threshold_gain_;
          self.prev_modified_trend_ = modified_trend;
          
          if modified_trend > self.threshold_ {
            if self.time_over_using_ == -1.0 {
              // Initialize the timer. Assume that we've been
              // over-using half of the time since the previous
              // sample.
              self.time_over_using_ = ts_delta / 2.0;
            } else {
              // Increment timer
              self.time_over_using_ += ts_delta;
            }
            self.overuse_counter_+=1;
            if (self.time_over_using_ > self.overusing_time_threshold_ && self.overuse_counter_ > 1) {
              if trend >= self.prev_trend_ {
                self.time_over_using_ = 0.0;
                self.overuse_counter_ = 0;
                self.hypothesis_ = BandwidthUsage::kBwOverusing;
              }
            }
          } else if modified_trend < -self.threshold_ {
            self.time_over_using_ = -1.0;
            self.overuse_counter_ = 0;
            self.hypothesis_ = BandwidthUsage::kBwUnderusing;
          } else {
            self.time_over_using_ = -1.0;
            self.overuse_counter_ = 0;
            self.hypothesis_ = BandwidthUsage::kBwNormal;
          }
          self.current_threshold_for_testing=self.threshold_;
          self.current_trend_for_testing=modified_trend;
          self.prev_trend_ = trend;

        }
        
      pub fn UpdateTrendline(&mut self,recv_delta_ms:f64,
          send_delta_ms:f64,
          send_time_ms:i64,
          arrival_time_ms:i64,
          packet_size:i64) {
                  let delta_ms = recv_delta_ms - send_delta_ms;
                  self.num_of_deltas_+=1;
                  self.num_of_deltas_ = std::cmp::min(self.num_of_deltas_, 1000);
                  if self.first_arrival_time_ms_ == -1{
                      self.first_arrival_time_ms_ = arrival_time_ms;
                  }
                  
                  // Exponential backoff filter.
                  self.accumulated_delay_ += delta_ms;
                  self.smoothed_delay_ = self.smoothing_coef_ * self.smoothed_delay_ +
                  (1.0 - self.smoothing_coef_) * self.accumulated_delay_;
                  
                  // Maintain packet window
                  self.delay_hist_.push_back(PacketTiming::new((arrival_time_ms-self.first_arrival_time_ms_) as f64,self.smoothed_delay_,self.accumulated_delay_));
                  if self.delay_hist_.len() > WINDOW_SIZE {
                      self.delay_hist_.pop_front();
                  }
                  
                  // Simple linear regression.
                  let mut  trend = self.prev_trend_;
                  if self.delay_hist_.len() == WINDOW_SIZE {
                  // Update trend_ if it is possible to fit a line to the data. The delay
                  // trend can be seen as an estimate of (send_rate - capacity)/capacity.
                  // 0 < trend < 1   ->  the delay increases, queues are filling up
                  //   trend == 0    ->  the delay does not change
                  //   trend < 0     ->  the delay decreases, queues are being emptied
                  trend = LinearFitSlope(&self.delay_hist_).unwrap_or(trend);
                  
                  }
                  self.Detect(trend, send_delta_ms, arrival_time_ms);
          }
                  
                  
  }

pub struct LinkCapacityEstimator{
    pub estimate_c: Option<f32>,
    pub deviation_c: f32,
}
impl LinkCapacityEstimator{
    pub fn new()->Self{
        Self{
            estimate_c:Option::None,
            deviation_c:0.4,
        }
    }

    pub fn deviation_estimate_c(&mut self)->f32 {
        return (self.deviation_c*self.estimate_c.unwrap()).sqrt();
    }

    pub fn  UpperBound(&mut self)->f32 {
        if !self.estimate_c.is_none()
        {
            return (self.estimate_c.unwrap()+ 3.0 * self.deviation_estimate_c());
        }
          
        return f32::INFINITY;
    }
      
    pub fn LowerBound(&mut self)->f32 {
        if !self.estimate_c.is_none(){
            return f32::max(0.1, self.estimate_c.unwrap()-3.0*self.deviation_estimate_c());
        }
          
        return 0.1;
    }

    pub fn Reset(&mut self){
        self.estimate_c=Option::None;
    }
      
    pub fn OnOveruseDetected(&mut self, acknowledged_c:f32) {
        self.Update(acknowledged_c, 0.05);
    }

    pub fn Update(&mut self,capacity_sample:f32, alpha:f32) {
        if self.estimate_c.is_none() {
          self.estimate_c = Some(capacity_sample);
        } else {
          self.estimate_c = Some((1.0 - alpha) * self.estimate_c.unwrap() + alpha * capacity_sample);
        }

        let norm = f32::max(self.estimate_c.unwrap(), 1.0);
        let mut error_kbps = self.estimate_c.unwrap() - capacity_sample;
        self.deviation_c =
            (1.0 - alpha) * self.deviation_c + alpha * error_kbps * error_kbps / norm;

        if self.deviation_c>2.5 as f32{
            self.deviation_c=2.5;
        }else if self.deviation_c<0.4 as f32{
            self.deviation_c=0.4;
        }
        
      }
      
      pub fn has_estimate(&mut self) ->bool {
        return !self.estimate_c.is_none();
      }
      
      pub fn  estimate(&mut self) ->f32 {
        return self.estimate_c.unwrap();
      }
}
pub struct EyeNexus_Controller{
    pub controller_c : f32,//27 - 188
    pub action : i32,//decrease 0, hold 1, increase 2
    pub trendline_manager : TrendlineEstimator,
    pub last_frame_arrival_timestamp : f64,
    pub last_frame_send_timestamp : f64,
    pub send_delta : f64,
    pub arrival_delta : f64,
    pub last_frame_ts : Duration,
    pub link_capacity_:LinkCapacityEstimator,
    pub is_complexity_recovery: bool,
    pub last_frame_size: i64,
}
impl EyeNexus_Controller {
    pub fn new()->Self{
        Self{
            controller_c : 188.,
            action : 1,
            trendline_manager : TrendlineEstimator::new(),
            last_frame_arrival_timestamp : 0.,
            last_frame_send_timestamp : 0.,
            send_delta : 0.,
            arrival_delta : 0.,
            last_frame_ts : Duration::ZERO,
            link_capacity_:LinkCapacityEstimator::new(),
            is_complexity_recovery: false,
            last_frame_size: 0,
        }
    }
    // EyeNexus-Gaze-Contingent Rate Control: Update the Foveation Controller C
    // Based on the detected network state (Normal, Overusing, Underusing), adjust C using AIMD logic (Sec 3.6).
    pub fn Update(&mut self, current_frame_send_timestamp: f64, current_frame_arrival_timestamp: f64, current_frame_size: i64, frame_target_ts : Duration, send_delta_in:i64,arrival_delta_in:i64)-> f32{

        let mut send_delta_ms= 0.0;
        let mut recv_delta_ms = 0.0;
        
        if self.last_frame_send_timestamp!=0.{
            send_delta_ms = (current_frame_send_timestamp - self.last_frame_send_timestamp)*0.001;
            recv_delta_ms = (current_frame_arrival_timestamp - self.last_frame_arrival_timestamp)*0.001;
        }
        send_delta_ms = (send_delta_in as f64)*0.001;
        recv_delta_ms = (arrival_delta_in as f64)*0.001;
        let adjust_current_frame_arrival_timestamp = self.last_frame_arrival_timestamp + arrival_delta_in as f64;
        let adjust_current_frame_send_timestamp = self.last_frame_send_timestamp + send_delta_in as f64;
        self.last_frame_send_timestamp = adjust_current_frame_send_timestamp;
        self.last_frame_arrival_timestamp = adjust_current_frame_arrival_timestamp;
        let send_time_ms = (adjust_current_frame_send_timestamp*0.001) as i64;
        let arrival_time_ms = (adjust_current_frame_arrival_timestamp*0.001) as i64;
        let packet_size = current_frame_size;
        self.send_delta = send_delta_ms;
        self.arrival_delta = recv_delta_ms;
        
        // Update the trendline based on delay gradients
        self.trendline_manager.UpdateTrendline(recv_delta_ms, send_delta_ms, send_time_ms, arrival_time_ms, packet_size);
        
        // Determine action based on bandwidth usage hypothesis
        if self.trendline_manager.hypothesis_ == BandwidthUsage::kBwNormal{
            if self.action == 1{
                self.action = 2; // Increase
            }
            else if self.action == 0{
                self.action = 1; // Hold
            }
        }else if self.trendline_manager.hypothesis_ == BandwidthUsage::kBwOverusing{
            if  self.action != 0{
                self.action = 0 // Decrease
            }
        }else{
            self.action = 1; // Hold
        }
        
        self.last_frame_size = current_frame_size;

        // EyeNexus-AIMD: Adjust Controller C
        if self.action == 0{
            // Multiplicative Decrease (beta = 0.9)
            self.controller_c = (self.controller_c as f32 *0.9);
            if self.controller_c < self.link_capacity_.LowerBound(){
                self.link_capacity_.Reset();
            }
            self.link_capacity_.OnOveruseDetected(self.controller_c);
            //self.action = 1;
        }else if self.action == 2{
            // Additive Increase (alpha = 0.1 or 0.2 depending on capacity estimate)
            if self.controller_c > self.link_capacity_.UpperBound(){
                self.link_capacity_.Reset();
            }
            if self.link_capacity_.has_estimate(){
                self.controller_c += 0.1;
            }else{
                self.controller_c += 0.2;
            }
            
        } 
        // Clamp C within bounds [C_min, C_max]
        if self.controller_c >80.{
            self.controller_c = 80.;
        }else if self.controller_c < 2.{
            self.controller_c = 2.;
        }
        return self.controller_c;
    }
    
}