// ROS message includes
#include "ros/ros.h"
#include <visualization_msgs/MarkerArray.h>
#include <cob_relayboard/EmergencyStopState.h>
#include <nav_msgs/Odometry.h>
#include <cob_srvs/SetInt.h>

/* protected region user include files on begin */
#include <sick_flexisoft_client/flexiclient.h>
#include <boost/static_assert.hpp>
/* protected region user include files end */

class safety_controller_config
{
public:
    std::string port;
    std::string host;
    double threshold_linear_slow;
    double threshold_linear_fast;
    double threshold_angular_fast;
};

class safety_controller_data
{
// autogenerated: don't touch this class
public:
    //input data
    nav_msgs::Odometry in_odometry;
    //output data
    visualization_msgs::MarkerArray out_marker;
    bool out_marker_active;
    cob_relayboard::EmergencyStopState out_emergency_stop_state;
    bool out_emergency_stop_state_active;
};

class safety_controller_impl
{
    /* protected region user member variables on begin */
	struct FlexiInput{
	    /*0.0*/ uint8_t gateway_not_connected:1;
	    /*0.1*/ uint8_t laser_stop_ok:1;
	    /*0.2*/ uint8_t edm_error_base:1;
	    /*0.3*/ uint8_t:1;
	    /*0.4*/ uint8_t:1;
	    /*0.5*/ uint8_t:1;
	    /*0.6*/ uint8_t:1;
	    /*0.7*/ uint8_t:1;

	    /*1.0 to 3.7*/ uint8_t _bytes_1_to_3[3-1+1];

	    /*4.0*/ uint8_t external_stop_ok:1;
	    /*4.1*/ uint8_t:1;
	    /*4.2*/ uint8_t:1;
	    /*4.3*/ uint8_t:1;
	    /*4.4*/ uint8_t edm_base:1;
	    /*4.5*/ uint8_t:1;
	    /*4.6*/ uint8_t:1;
	    /*4.7*/ uint8_t wireless_emstop_ok:1;

	    /*5.0 to 15.7*/ uint8_t _bytes_5_to_15[15-5+1];

	    /*16.0*/ uint8_t base_active:1;
	    /*16.1*/ uint8_t:1;
	    /*16.2*/ uint8_t torso_active:1;
	    /*16.3*/ uint8_t:1;
	    /*16.4*/ uint8_t:1;
	    /*16.5*/ uint8_t:1;
	    /*16.6*/ uint8_t:1;
	    /*16.7*/ uint8_t:1;

	    /*17.0 to 27.7*/ uint8_t _bytes_17_to_27[27-17+1];

	    /*28.0*/ uint8_t:1;
	    /*28.1*/ uint8_t:1;
	    /*28.2*/ uint8_t:1;
	    /*28.3*/ uint8_t:1;
	    /*28.4*/ uint8_t front_1_blocked:1;
	    /*28.5*/ uint8_t front_2_blocked:1;
	    /*28.6*/ uint8_t:1;
	    /*28.7*/ uint8_t front_3_blocked:1;

	    /*29.0*/ uint8_t:1;
	    /*29.1*/ uint8_t:1;
	    /*29.2*/ uint8_t:1;
	    /*29.3*/ uint8_t:1;
	    /*29.4*/ uint8_t left_1_blocked:1;
	    /*29.5*/ uint8_t left_2_blocked:1;
	    /*29.6*/ uint8_t:1;
	    /*29.7*/ uint8_t left_3_blocked:1;

	    /*30.0*/ uint8_t:1;
	    /*30.1*/ uint8_t:1;
	    /*30.2*/ uint8_t:1;
	    /*30.3*/ uint8_t:1;
	    /*30.4*/ uint8_t right_1_blocked:1;
	    /*30.5*/ uint8_t right_2_blocked:1;
	    /*30.6*/ uint8_t:1;
	    /*30.7*/ uint8_t right_3_blocked:1;

	}  __attribute__ ((__packed__));
	BOOST_STATIC_ASSERT_MSG(sizeof(FlexiInput) <= sizeof(flexi::FlexiInputData::data), "FlexiInput does not fit into payload");
	BOOST_STATIC_ASSERT_MSG(sizeof(FlexiInput) == 31, "FlexiIntput is not 31 bytes");

	struct FlexiOutput{
	    /*0.0*/ uint8_t laser_case:5;
	    /*0.5*/ uint8_t far_front:1;
	    /*0.6*/ uint8_t far_left:1;
	    /*0.7*/ uint8_t far_right:1;

	    /*1.0 to 1.1*/ uint8_t enable_components:2; // base: bit 0, torso: bit 1
	    /*1.2*/ uint8_t:1;
	    /*1.3*/ uint8_t:1;
	    /*1.4*/ uint8_t:1;
	    /*1.5*/ uint8_t:1;
	    /*1.6*/ uint8_t:1;
	    /*1.7*/ uint8_t:1;
	    }  __attribute__ ((__packed__));
	BOOST_STATIC_ASSERT_MSG(sizeof(FlexiOutput) <= sizeof(flexi::FlexiOutputData::data), "FlexiOutput does not fit into payload");

	boost::scoped_ptr <flexi::FlexiClient> flexi_client_;
	FlexiInput flexi_input_;
	boost::mutex flexi_input_mutex_;
	FlexiOutput *flexi_output_p_; // direct access to protocol data
	flexi::FlexiMsg flexi_output_msg_;

	/* protected region user member variables end */

public:
    safety_controller_impl() 
    {
		/* protected region user constructor on begin */
		flexi_client_.reset(new flexi::FlexiClient(boost::bind(&safety_controller_impl::handle_input, this, _1)));
		memset(&flexi_input_, sizeof(flexi_input_), 0);

		BOOST_STATIC_ASSERT_MSG(sizeof(FlexiOutput) <= 10, "FlexiOutput does not fit into first field with 10 bytes");

		// set up output message with one field
		std::vector<uint8_t> output_fields[5] = { std::vector<uint8_t>(sizeof(FlexiOutput)), std::vector<uint8_t>(0), std::vector<uint8_t>(0), std::vector<uint8_t>(0), std::vector<uint8_t>(0) };
		flexi_output_msg_.set_output(output_fields);
		
		flexi_output_p_ = (FlexiOutput*) flexi_output_msg_.payload.output.data; 
		
		flexi_output_p_->enable_components = 3; // base and torso enabled
		/* protected region user constructor end */
    }

    void configure(safety_controller_config config) 
    {
        /* protected region user configure on begin */
		if (!flexi_client_->connect(config.host, config.port))
		{
			exit (EXIT_FAILURE);
		}
		flexi_client_->start_worker();
		flexi_client_->start_worker();
		
		BOOST_STATIC_ASSERT_MSG(sizeof(FlexiInput) <= 32, "FlexiInput does not fit into first field with 32 bytes");
		flexi::FlexiMsg control;
		// flexisoft should send update each 1000 ms or if set 1 has changed
		control.set_control(true, false, false,false, 1000); 
		flexi_client_->send(control);
		
		flexi_client_->send(flexi_output_msg_); // send initial values

		/* protected region user configure end */
    }

    void update(safety_controller_data &data, safety_controller_config config)
    {
        /* protected region user update on begin */
double lin_velocity = sqrt(data.in_odometry.twist.twist.linear.y*data.in_odometry.twist.twist.linear.y + data.in_odometry.twist.twist.linear.x*data.in_odometry.twist.twist.linear.x);
		double rot_velocity = fabs(data.in_odometry.twist.twist.angular.z);

		// reset far flags
		if(rot_velocity < config.threshold_angular_fast){
			flexi_output_p_->far_front = 0;
			flexi_output_p_->far_left = 0;
			flexi_output_p_->far_right = 0;
		}else{
			flexi_output_p_->far_front = 1;
			flexi_output_p_->far_left = 1;
			flexi_output_p_->far_right = 1;
		}

		if(lin_velocity < config.threshold_linear_slow){ // does not move in linear direction
			flexi_output_p_->laser_case = 0; // case = rotate
		}else{

			double direction = atan2(data.in_odometry.twist.twist.linear.y, data.in_odometry.twist.twist.linear.x) /M_PI*180.0 + 7.5; // direction lower bound, aligned to 15 deg
			if(direction < 0) direction += 360; // enforce [0,360) range
			flexi_output_p_->laser_case = (int(direction/15) % 24) + 1; // select case  in [1,24]

			// principal directions: front  1, left  9, right  17
			if( flexi_output_p_->laser_case  < 9 || flexi_output_p_->laser_case > 17){ // drive mode for front
				flexi_output_p_->far_front = lin_velocity < config.threshold_linear_fast ? 0 : 1;
			}
			if( flexi_output_p_->laser_case  > 1 && flexi_output_p_->laser_case < 17){  // drive mode for left
				flexi_output_p_->far_left = lin_velocity < config.threshold_linear_fast ? 0 : 1;
			}
			if( flexi_output_p_->laser_case  > 9){ // drive mode for right
				flexi_output_p_->far_right = lin_velocity < config.threshold_linear_fast ? 0 : 1;
			}
		}

		flexi_client_->send(flexi_output_msg_);

		// set emergency information
		data.out_emergency_stop_state.emergency_button_stop = !flexi_input_.external_stop_ok || !flexi_input_.wireless_emstop_ok;
		data.out_emergency_stop_state.scanner_stop = !flexi_input_.laser_stop_ok;
		if (!data.out_emergency_stop_state.emergency_button_stop && !data.out_emergency_stop_state.scanner_stop)
			data.out_emergency_stop_state.emergency_state = data.out_emergency_stop_state.EMFREE;
		else
			data.out_emergency_stop_state.emergency_state = data.out_emergency_stop_state.EMSTOP;

		// TODO add marker

		/* protected region user update end */
    }

    bool callback_set_mode(cob_srvs::SetInt::Request  &req, cob_srvs::SetInt::Response &res , safety_controller_config config)
    {
        /* protected region user implementation of service callback for set_mode on begin */
    	switch (req.data){
    	case 0:
    	case 1:
    	case 2:
    	case 3:
    		flexi_output_p_->enable_components = req.data;
    		res.success = true;
    		break;
    	default:
    		res.success = false;
    	}
		/* protected region user implementation of service callback for set_mode end */
        return true;
    }

    /* protected region user additional functions on begin */
	void handle_input(const flexi::FlexiInputData &input)
	{
		boost::mutex::scoped_lock lock(flexi_input_mutex_);
	    flexi_input_ = *(FlexiInput*)input.data;
	}
	/* protected region user additional functions end */
};