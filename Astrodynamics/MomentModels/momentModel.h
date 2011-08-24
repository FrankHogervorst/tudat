/*! \file momentModel.h
 *    This header file contains the moment model base class included in Tudat.
 *
 *    Path              : /Astrodynamics/ForceAndMomentModels/
 *    Version           : 2
 *    Check status      : Checked
 *
 *    Checker           : F. M. Engelen
 *    Affiliation       : Delft University of Technology
 *    E-mail address    : F.M.Engelen@student.tudelft.nl
 *
 *    Checker           : K. Kumar
 *    Affiliation       : Delft University of Technology
 *    E-mail address    : K.Kumar@tudelft.nl
 *
 *    Date created      : 10 May, 2011
 *    Last modified     : 10 August, 2011
 *
 *    References
 *
 *    Notes
 *      This baseclass already has the architecture to calculate the extra
 *      moment due to a force, in other words:
 *     total moment = arm x force + moment.
 *
 *    Copyright (c) 2010 Delft University of Technology.
 *
 *    This software is protected by national and international copyright.
 *    Any unauthorized use, reproduction or modification is unlawful and
 *    will be prosecuted. Commercial and non-private application of the
 *    software in any form is strictly prohibited unless otherwise granted
 *    by the authors.
 *
 *    The code is provided without any warranty; without even the implied
 *    warranty of merchantibility or fitness for a particular purpose.
 *
 *    Changelog
 *      YYMMDD    Author            Comment
 *      110510    F.M. Engelen      First creation of code.
 *      110810    K. Kumar          Minor corrections; changed function names
 *                                  and removed redundant functions.
 */

#ifndef MOMENTMODEL_H
#define MOMENTMODEL_H

// Include statements.
#include "forceModel.h"
#include "linearAlgebra.h"

//! Base class for moment models.
/*!
 * Base class for the moment models used in Tudat.
 */
class MomentModel
{
public:

    //! Default constructor.
    /*!
     * Default constructor.
     */
    MomentModel( );

    //! Default destructor.
    /*!
     * Default destructor.
     */
    virtual ~MomentModel( );

    //! Set force application point.
    /*!
     * Sets force application arm, i.e., the vector from the origin of the reference frame in
     * which the moment is calculated, to the application point of the input force.
     * \param forceApplicationPoint Vector arm to application point of force.
     */
    void setForceApplicationArm( Vector3d& forceApplicationArm );

    //! Get force application arm.
    /*!
     * Returns force application arm, i.e., returns the vector from the origin of the reference
     * frame in which the moment is calculated, to the application point of the input force.
     * \return Vector arm to application point of force.
     */
    Vector3d& getForceApplicationArm( );

    //! Set force model.
    /*!
     * Sets the force model. This force should be calculated in a reference frame that has the same
     * orientation as the frame the moment is calculated in (normally the body frame).
     * \param pointerToForceModel Pointer to force model.
     */
    void setForceModel( ForceModel* pointerToForceModel );

    //! Get force model.
    /*!
     * Returns force model. This force is calculated in a reference frame that has the same
     * orientation as the frame the moment is calculated in (normally the body frame).
     * \return Pointer to force model.
     */
    ForceModel* getForceModel( );

    //! Get moment.
    /*!
     * Returns the moment.
     * \return Moment.
     */
    Vector3d& getMoment( );

    //! Set moment.
    /*!
     * Sets the moment.
     * \param moment Moment.
     */
    void setMoment( Vector3d& moment );

    //! Compute moment.
     /*!
      * Computes the moment.
      */
    virtual void computeMoment( State* pointerToState ) = 0;

protected:

    //! Pointer to force model.
    /*!
     * Pointer to force model.
     */
    ForceModel* pointerToForceModel_;

    //! Moment.
    /*!
     * Moment.
     */
    Vector3d moment_;

    //! Force application point.
    /*!
     * Force application point, which can also be considered the force arm.
     */
    Vector3d forceApplicationArm_;

private:
};

#endif // MOMENTMODEL_H

// End of file.