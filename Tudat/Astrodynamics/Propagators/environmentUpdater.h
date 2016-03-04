#ifndef ENVIRONMENTUPDATER_H
#define ENVIRONMENTUPDATER_H

#include <vector>
#include <string>
#include <map>

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "Tudat/SimulationSetup/body.h"
#include "Tudat/Astrodynamics/Propagators/propagationSettings.h"

namespace tudat
{

namespace propagators
{

//! Enum defining types of environment model updates that can be done.
enum EnvironmentModelsToUpdate
{
    body_transational_state_update = 0,
    body_rotational_state_update = 1,
    body_mass_update = 2,
    vehicle_flight_conditions_update = 3,
    radiation_pressure_interface_update = 4,
};

//! Class used to update the environment during numerical integration.
/*!
 *  Class used to update the environment during numerical integration. The class ensures that the
 *  current state of the numerical integration is properly set, and that all the environment models
 *  that are used during the numerical integration are updated to the current time and state in the
 *  correct order.
 */
template< typename StateScalarType, typename TimeType >
class EnvironmentUpdater
{
public:

    //! Constructor
    /*!
     * Constructor, provides the required settings for updating the environment.
     * \param bodyList List of body objects, this list encompasses all environment object in the
     * simulation.
     * \param updateSettings List of updates of the environment models that are required.
     * The list defines per model type (key) the bodies for which this environment model should be
     * updated (values)
     * \param integratedStates This map provides the list of identifiers for the numerically
     * integrated states. The type of integrated state (key) is defined for each reference
     * (value). Note that for the numerical integration of translational motion, the entry
     * in the pair will have a second entry that is empty (""), with the first entry defining
     * the body that is integrated.
     */
    EnvironmentUpdater(
            const simulation_setup::NamedBodyMap& bodyList,
            const std::map< EnvironmentModelsToUpdate, std::vector< std::string > >& updateSettings,
            const std::map< IntegratedStateType,
            std::vector< std::pair< std::string, std::string > > >& integratedStates =
            ( std::map< IntegratedStateType,
              std::vector< std::pair< std::string, std::string > > >( ) ) ):
        bodyList_( bodyList ), integratedStates_( integratedStates )
    {
        // Set update function to be evaluated as dependent variables of state and time during each
        // integration time step.
        setUpdateFunctions( updateSettings );
    }

    //! Function to update the environment to the current state and time.
    /*!
     * Function to update the environment to the current state and time. This function calls the dependent variable
     * functions set by the setUpdateFunctions function. By default, the numerically integrated states are set in the
     * environment first. This may be overridden by using the setIntegratedStatesFromEnvironment variable, which forces
     * the function to ignore specific integrated states and update them from the existing environment models instead.
     * \param currentTime Current time.
     * \param integratedStatesToSet Current list of integrated states, with specific integrated states defined by
     * integratedStates_ member variable. Note that these states must have been converted to the global state before
     * input to this function, as is done by the getStatesPerTypeInConventionalRepresentation function of the
     * DynamicsStateDerivativeModel.
     * \param setIntegratedStatesFromEnvironment Integrated state types which are not to be used for updating the
     * environment, but which are to be set from existing environment models instead.
     */
    void updateEnvironment(
            const TimeType currentTime,
            const std::map< IntegratedStateType, Eigen::Matrix< StateScalarType, Eigen::Dynamic, 1 > >& integratedStatesToSet,
            const std::vector< IntegratedStateType >& setIntegratedStatesFromEnvironment = std::vector< IntegratedStateType >( ) )
    {
        // Check consistency of input.
        if( integratedStatesToSet.size( ) + setIntegratedStatesFromEnvironment.size( ) != integratedStates_.size( ) )
        {
            throw std::runtime_error( "Error when updating environment, input size is inconsistent " +
                                      boost::lexical_cast< std::string >( integratedStatesToSet.size( ) ) + " " +
                                      boost::lexical_cast< std::string >( setIntegratedStatesFromEnvironment.size( ) ) + " " +
                                      boost::lexical_cast< std::string >( integratedStates_.size( ) ) );
        }

        // Set integrated state variables in environment.
        setIntegratedStatesInEnvironment( integratedStatesToSet );

        // Set current state from environment for override settings setIntegratedStatesFromEnvironment
        setStatesFromEnvironment( setIntegratedStatesFromEnvironment, currentTime );

        // Set states of bodies which are not numerically integrated .
        for( outerCurrentStateFromEnvironmentIterator_ = currentStateFromEnvironmentList_.begin( );
             outerCurrentStateFromEnvironmentIterator_ != currentStateFromEnvironmentList_.end( );
             outerCurrentStateFromEnvironmentIterator_++ )
        {
            for( currentStateFromEnvironmentIterator_ = outerCurrentStateFromEnvironmentIterator_->second.begin( );
                 currentStateFromEnvironmentIterator_ != outerCurrentStateFromEnvironmentIterator_->second.end( );
                 currentStateFromEnvironmentIterator_++ )
            {
                currentStateFromEnvironmentIterator_->second( currentTime );
            }
        }

        // Evaluate update functions (dependent variables of state and time) determined by setUpdateFunctions
        for( updateFunctionIterator = updateFunctionList_.begin( ); updateFunctionIterator != updateFunctionList_.end( );
             updateFunctionIterator++ )
        {
            for( unsigned int i = 0; i < updateFunctionIterator->second.size( ); i++ )
            {
                updateFunctionIterator->second.at( i ).second( );
            }
        }

        // Evaluate time-dependent update functions (dependent variables of state and time) determined by setUpdateFunctions
        for( updateTimeIterator = updateTimeFunctionList_.begin( ); updateTimeIterator != updateTimeFunctionList_.end( );
             updateTimeIterator++ )
        {
            for( unsigned int i = 0; i < updateTimeIterator->second.size( ); i++ )
            {
                updateTimeIterator->second.at( i ).second( static_cast< double >( currentTime ) );
            }
        }
    }

private:

    //! Function to set numerically integrated states in environment.
    /*!
     * Function to set numerically integrated states in environment.
     * \param integratedStatesToSet Integrated states which are to be set in environment.
     * Note that these states must have been converted to the global state before
     * input to this function, as is done by the getStatesPerTypeInConventionalRepresentation function of the
     * DynamicsStateDerivativeModel.
     */
    void setIntegratedStatesInEnvironment(
            const std::map< IntegratedStateType, Eigen::Matrix< StateScalarType, Eigen::Dynamic, 1 > >&
            integratedStatesToSet )
    {
        // Iterate over state types and set states in environment
        for( typename std::map< IntegratedStateType, Eigen::Matrix< StateScalarType, Eigen::Dynamic, 1 > >::const_iterator
             integratedStateIterator = integratedStatesToSet.begin( );
             integratedStateIterator != integratedStatesToSet.end( );
             integratedStateIterator++ )
        {
            switch( integratedStateIterator->first )
            {
            case transational_state:
            {
                // Set translational states for bodies provided as input.
                std::vector< std::pair< std::string, std::string > > bodiesWithIntegratedStates =
                        integratedStates_.at( transational_state );
                for( unsigned int i = 0; i < bodiesWithIntegratedStates.size( ); i++ )
                {
                    bodyList_[ bodiesWithIntegratedStates[ i ].first ]->template setTemplatedState< StateScalarType >(
                                integratedStateIterator->second.segment( i * 6, 6 ) );
                }
                break;
            };
            default:
                throw std::runtime_error( "Error, could not find integrated state settings for " +
                                          boost::lexical_cast< std::string >( integratedStateIterator->first ) );
            }
        }
    }
    //! Function to explicitly use existing environment models to update current states of integrated bodies
    /*!
     * Function to explicitly use existing environment models to update current states of integrated bodies, overriding
     * the numerically integrated states .
     * \param integratedStatesToSet Integrated state types which are not to be used for updating the
     * environment, but which are to be set from existing environment models instead.
     */
    void setStatesFromEnvironment(
            const std::vector< IntegratedStateType >& statesToSet,
            const TimeType currentTime )
    {
        // Iterate over selected state types.
        for( unsigned int i = 0; i < statesToSet.size( ); i++ )
        {
            switch( statesToSet.at( i ) )
            {
            case transational_state:
            {
                // Iterate over all integrated translational states.
                std::vector< std::pair< std::string, std::string > > bodiesWithIntegratedStates =
                        integratedStates_.at( transational_state );
                for( unsigned int i = 0; i < bodiesWithIntegratedStates.size( ); i++ )
                {
                    bodyList_[ bodiesWithIntegratedStates[ i ].first ]->
                            template setTemplatedStateFromEphemeris< StateScalarType, TimeType >( currentTime );

                }
                break;
            }
            default:
                throw std::runtime_error( "Error, could not find  state settings for " +
                                          boost::lexical_cast< std::string >( statesToSet.at( i ) ) );
            }
        }
    }

    void setUpdateFunctions( const std::map< EnvironmentModelsToUpdate, std::vector< std::string > >& updateSettings )
    {

        for( std::map< EnvironmentModelsToUpdate, std::vector< std::string > >::const_iterator updateIterator =
             updateSettings.begin( ); updateIterator != updateSettings.end( ); updateIterator++ )
        {
            std::vector< std::string > currentBodies = updateIterator->second;
            for( unsigned int i = 0; i < currentBodies.size( ); i++ )
            {
                if( currentBodies.at( i ) != "" )
                {
                    if( bodyList_.count( currentBodies.at( i ) ) == 0 )
                    {
                        throw std::runtime_error(
                                    "Error when setting environment update functions, could not find body " +
                                    currentBodies.at( i ) );
                    }
                    switch( updateIterator->first )
                    {
                    case body_transational_state_update:
                    {
                        bool addUpdate = 1;
                        if( integratedStates_.count( transational_state ) > 0 )
                        {
                            std::pair< std::string, std::string > bodyToCheck = std::make_pair( currentBodies.at( i ), "" );
                            std::vector< std::pair< std::string, std::string > > integratedTranslationalStates =
                                    integratedStates_.at( transational_state );
                            if( std::find( integratedTranslationalStates.begin( ), integratedTranslationalStates.end( ), bodyToCheck ) !=
                                    integratedTranslationalStates.end( ) )
                            {
                                addUpdate = 0;
                            }
                        }
                        if( addUpdate == 1 )
                        {
                            boost::function< void( const TimeType ) > stateSetFunction =
                                    boost::bind( &simulation_setup::Body::setTemplatedStateFromEphemeris< StateScalarType, TimeType >,
                                                 bodyList_.at( currentBodies.at( i ) ), _1 );

                            currentStateFromEnvironmentList_[ body_transational_state_update ].insert( std::make_pair( currentBodies.at( i ), stateSetFunction ) );
                        }
                        break;
                    }
                    case body_rotational_state_update:
                    {

                        boost::shared_ptr< ephemerides::RotationalEphemeris > rotationalEphemeris =
                                bodyList_.at( currentBodies.at( i ) )->getRotationalEphemeris( );
                        if( rotationalEphemeris != NULL )
                        {
                            boost::function< void( const TimeType ) > rotationalStateSetFunction =
                                    boost::bind( &simulation_setup::Body::setCurrentRotationalStateToLocalFrameFromEphemeris,
                                                 bodyList_.at( currentBodies.at( i ) ), _1 );
                            currentStateFromEnvironmentList_[ body_rotational_state_update ].insert( std::make_pair( currentBodies.at( i ), rotationalStateSetFunction ) );
                        }

                        break;

                    }
                    case body_mass_update:
                    {
                        updateTimeFunctionList_[ body_mass_update ].push_back(
                                    std::make_pair( currentBodies.at( i ),
                                                    boost::bind( &simulation_setup::Body::updateMass, bodyList_.at( currentBodies.at( i ) ), _1  ) ) );
                        break;
                    }
                    case vehicle_flight_conditions_update:
                    {
                        // Check if current body has flight conditions set.
                        if( bodyList_.at( currentBodies.at( i ) )->getFlightConditions( ) != NULL )
                        {
                            // If vehicle has flight conditions, add flight conditions update function to update list.
                            updateFunctionList_[ vehicle_flight_conditions_update ].push_back(
                                        std::make_pair(
                                            currentBodies.at( i ), boost::bind( &aerodynamics::FlightConditions::updateConditions,
                                                                                bodyList_.at( currentBodies.at( i ) )->getFlightConditions( ) ) ) );
                        }
                        else
                        {
                            throw std::runtime_error(
                                        "Request flight condition update of " + currentBodies.at( i ) +
                                        ", but body has no flight conditions" );
                        }
                        break;
                    }

                    case radiation_pressure_interface_update:
                    {
                        // Get body radiation pressure interface(s) (one per source)
                        std::map< std::string, boost::shared_ptr< electro_magnetism::RadiationPressureInterface > > radiationPressureInterfaces =
                                bodyList_.at( currentBodies.at( i ) )->getRadiationPressureInterfaces( );

                        if( radiationPressureInterfaces.size( ) == 0 )
                        {
                            throw std::runtime_error(
                                        "Request radiation pressure update of " + currentBodies.at( i ) +
                                        ", but body has no radiation pressure interfaces" );
                        }
                        else if( radiationPressureInterfaces.size( ) > 1 )
                        {
                            std::cerr<<"Request radiation pressure update of "<<currentBodies.at( i )<<
                                       ", but body has multiple radiation pressure interfaces: updating all."<<std::endl;
                        }

                        // Add each interface update function to update list.
                        for( std::map< std::string, boost::shared_ptr< electro_magnetism::RadiationPressureInterface > > ::iterator iterator =
                             radiationPressureInterfaces.begin( ); iterator != radiationPressureInterfaces.end( ); iterator++ )
                        {
                            updateTimeFunctionList_[ radiation_pressure_interface_update ].push_back(
                                        std::make_pair( currentBodies.at( i ),
                                                        boost::bind( &electro_magnetism::RadiationPressureInterface::updateInterface,
                                                                     iterator->second, _1 ) ) );
                        }
                        break;
                    }
                    }
                }
            }
        }
    }

    //! List of body objects, this list encompasses all environment object in the simulation.
    simulation_setup::NamedBodyMap bodyList_;


    //! list of identifiers for the numerically integrated states
    /*!
     * This map provides the list of identifiers for the numerically
     * integrated states. The type of integrated state (key) is defined for each reference
     * (value). Note that for the numerical integration of translational motion, the entry
     * in the pair will have a second entry that is empty (""), with the first entry defining
     * the body that is integrated.
     */
    std::map< IntegratedStateType, std::vector< std::pair< std::string, std::string > > >
    integratedStates_;


    std::map< EnvironmentModelsToUpdate, std::multimap< std::string, boost::function< void( const TimeType ) > > > currentStateFromEnvironmentList_;

    typename std::map< EnvironmentModelsToUpdate, std::multimap< std::string, boost::function< void( const TimeType ) > > >::iterator outerCurrentStateFromEnvironmentIterator_;

    typename std::multimap< std::string, boost::function< void( const TimeType ) > >::iterator currentStateFromEnvironmentIterator_;


    std::map< EnvironmentModelsToUpdate, std::vector< std::pair< std::string, boost::function< void( ) > > > > updateFunctionList_;

    std::map< EnvironmentModelsToUpdate, std::vector< std::pair< std::string, boost::function< void( ) > > > >::iterator updateFunctionIterator;


    std::map< EnvironmentModelsToUpdate, std::vector< std::pair< std::string, boost::function< void( const double ) > > > > updateTimeFunctionList_;

    std::map< EnvironmentModelsToUpdate, std::vector< std::pair< std::string, boost::function< void( const double ) > > > >::iterator updateTimeIterator;


};

}

}

#endif // ENVIRONMENTUPDATER_H
