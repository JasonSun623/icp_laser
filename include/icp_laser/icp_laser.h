

#include "ros/ros.h"

#include "geometry_msgs/Pose.h"
#include "geometry_msgs/PoseWithCovarianceStamped.h"
#include "sensor_msgs/LaserScan.h"
#include "nav_msgs/OccupancyGrid.h"

#include "tf/transform_listener.h"

#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/point_cloud.h>
#include <pcl/registration/icp.h>
#include "pcl_ros/point_cloud.h"

#include "occupancy_grid_utils/ray_tracer.h"

#include "laser_geometry/laser_geometry.h"

//Publish simulated laser scan for debug purposes
#define PUBLISH_SIMULATED_LASER_SCAN

#define PUBLISH_SIMULATED_LASER_CLOUD

#define PUBLISH_LASER_CLOUD

struct TransformWithFitness
{
	tf::Transform transform;
	double fitness;
};

class icp_laser
{

	//Stores the map taken from /map topic
	nav_msgs::OccupancyGrid map;

	//It is true if we get map from /map topic
	bool we_have_map;

	//Stores the laser scanner data taken from /scan topic
	sensor_msgs::LaserScan laser;
	pcl::PointCloud<pcl::PointXYZ>::Ptr laser_cloud;

	//It is true if we get laser data
	bool we_have_laser;

	tf::TransformListener tf_listener;
	tf::StampedTransform laser_to_base;
	tf::StampedTransform base_transform;
	
	ros::NodeHandle nodeHandle;
	ros::Subscriber map_subscriber;
	ros::Subscriber laser_subscriber;
	ros::Publisher pose_publisher;

	//Publishing data for visualization purposes
	#ifdef PUBLISH_SIMULATED_LASER_SCAN
	ros::Publisher sim_laser_publisher;
	#endif

	#ifdef PUBLISH_SIMULATED_LASER_CLOUD
	ros::Publisher sim_laser_cloud_publisher;
	#endif

	#ifdef PUBLISH_LASER_CLOUD
	ros::Publisher laser_cloud_publisher;
	ros::Publisher transformed_laser_cloud_publisher;
	#endif

	//Neglect laser data far away from that distance
	double max_simulated_point_distance;
	int min_simulated_point_count;
	double max_laser_point_distance;
	int min_laser_point_count;

	//ICP parameters
	double icp_max_correspondence_distance;
	unsigned int icp_max_iterations;
	double icp_transformation_epsilon;
	double icp_euclidean_distance_epsilon;

	double inlier_distance;

	double fitness_threshold;

	//Pose update parameters
	double max_jump_distance; //Do not publish the pose if it is too far away
	double min_jump_distance; //No need to publish new pose if it is too close
	double max_rotation; //Same for rotation
	double min_rotation;

	double update_interval;  //Limit the update frequency of pose
	ros::Time update_time;  //Holds the last time new pose published

	//covariance in published pose (PoseWithCovarianceStamped)
	double pose_covariance_xx;
	double pose_covariance_yy;
	double pose_covariance_aa;

	pcl::KdTree<pcl::PointXYZ>::Ptr target_tree;




public:
	icp_laser();

	//Create simulated laser data as if robot is at give pose
	sensor_msgs::LaserScan::Ptr createSimulatedLaserScan(geometry_msgs::Pose&);

	//Map callback
	void getMapFromTopic(const nav_msgs::OccupancyGrid::Ptr);
	//Laser scan callback
	void getLaserFromTopic(const sensor_msgs::LaserScan::Ptr);

	//get the current laser pose
	geometry_msgs::Pose currentPose();

	//performs the ICP algorithm and return the final transformation
	TransformWithFitness find(pcl::IterativeClosestPoint<pcl::PointXYZ, pcl::PointXYZ> &);


	void setICPParameters(double, unsigned int, double, double);
	void setLaserCloudParameters(double, int, double, int);
	void setJumpParameters(double, double, double, double);
	void setPoseCovariance(double, double, double);
	void setUpdateInterval(double);
	void setInlierThreshold(double);

	//Update the robot pose according to the given transform
	void updatePose(TransformWithFitness);

	//convert laser scan to point cloud, laser scanner pose is given
	void laserToPCloud(
		sensor_msgs::LaserScan&, 
		geometry_msgs::Pose, 
		pcl::PointCloud<pcl::PointXYZ>::Ptr&, 
		double,
		int);

	//convert transformation matrix to tf::Traansform object
	void matrixAsTransform (const Eigen::Matrix4f&,  tf::Transform&);
};


double abs(double d)
{
	if(d < 0) return -d;
	else return d;
}

