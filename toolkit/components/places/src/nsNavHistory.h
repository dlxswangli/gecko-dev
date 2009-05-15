/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Places code.
 *
 * The Initial Developer of the Original Code is
 * Google Inc.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Brett Wilson <brettw@gmail.com> (original author)
 *   Edward Lee <edward.lee@engineering.uiuc.edu>
 *   Ehsan Akhgari <ehsan.akhgari@gmail.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef nsNavHistory_h_
#define nsNavHistory_h_

#include "mozIStorageService.h"
#include "mozIStorageConnection.h"
#include "mozIStorageValueArray.h"
#include "mozIStorageStatement.h"
#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "nsDataHashtable.h"
#include "nsINavHistoryService.h"
#include "nsPIPlacesDatabase.h"
#include "nsPIPlacesHistoryListenersNotifier.h"
#ifdef MOZ_XUL
#include "nsIAutoCompleteController.h"
#include "nsIAutoCompleteInput.h"
#include "nsIAutoCompletePopup.h"
#include "nsIAutoCompleteSearch.h"
#include "nsIAutoCompleteResult.h"
#include "nsIAutoCompleteSimpleResult.h"
#endif
#include "nsIBrowserHistory.h"
#include "nsICollation.h"
#include "nsIGlobalHistory.h"
#include "nsIGlobalHistory3.h"
#include "nsIDownloadHistory.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsServiceManagerUtils.h"
#include "nsIStringBundle.h"
#include "nsITextToSubURI.h"
#include "nsITimer.h"
#ifdef MOZ_XUL
#include "nsITreeSelection.h"
#include "nsITreeView.h"
#endif
#include "nsString.h"
#include "nsWeakReference.h"
#include "nsTArray.h"
#include "nsINavBookmarksService.h"
#include "nsMaybeWeakPtr.h"

#include "nsNavHistoryExpire.h"
#include "nsNavHistoryResult.h"
#include "nsNavHistoryQuery.h"

#include "nsICharsetResolver.h"

#include "nsIPrivateBrowsingService.h"
#include "nsNetCID.h"

// define to enable lazy link adding
#define LAZY_ADD

#define QUERYUPDATE_TIME 0
#define QUERYUPDATE_SIMPLE 1
#define QUERYUPDATE_COMPLEX 2
#define QUERYUPDATE_COMPLEX_WITH_BOOKMARKS 3
#define QUERYUPDATE_HOST 4

// This magic number specified an uninitialized value for the
// mInPrivateBrowsing member
#define PRIVATEBROWSING_NOTINITED (PRBool(0xffffffff))

#define PLACES_INIT_COMPLETE_EVENT_TOPIC "places-init-complete"
#define PLACES_DB_LOCKED_EVENT_TOPIC "places-database-locked"

struct AutoCompleteIntermediateResult;
class AutoCompleteResultComparator;
class mozIAnnotationService;
class nsNavHistory;
class nsNavBookmarks;
class QueryKeyValuePair;
class nsIEffectiveTLDService;
class nsIIDNService;
class PlacesSQLQueryBuilder;

// nsNavHistory

class nsNavHistory : public nsSupportsWeakReference
                   , public nsINavHistoryService
                   , public nsIObserver
                   , public nsIBrowserHistory
                   , public nsIGlobalHistory3
                   , public nsIDownloadHistory
                   , public nsICharsetResolver
                   , public nsPIPlacesDatabase
                   , public nsPIPlacesHistoryListenersNotifier
#ifdef MOZ_XUL
                   , public nsIAutoCompleteSearch
                   , public nsIAutoCompleteSimpleResultListener
#endif
{
  friend class AutoCompleteIntermediateResultSet;
  friend class AutoCompleteResultComparator;
  friend class PlacesSQLQueryBuilder;

public:
  nsNavHistory();

  NS_DECL_ISUPPORTS

  NS_DECL_NSINAVHISTORYSERVICE
  NS_DECL_NSIGLOBALHISTORY2
  NS_DECL_NSIGLOBALHISTORY3
  NS_DECL_NSIDOWNLOADHISTORY
  NS_DECL_NSIBROWSERHISTORY
  NS_DECL_NSIOBSERVER
  NS_DECL_NSPIPLACESDATABASE
  NS_DECL_NSPIPLACESHISTORYLISTENERSNOTIFIER
#ifdef MOZ_XUL
  NS_DECL_NSIAUTOCOMPLETESEARCH
  NS_DECL_NSIAUTOCOMPLETESIMPLERESULTLISTENER
#endif


  /**
   * Obtains the nsNavHistory object.
   */
  static nsNavHistory *GetSingleton();

  /**
   * Initializes the nsNavHistory object.  This should only be called once.
   */
  nsresult Init();

  /**
   * Used by other components in the places directory such as the annotation
   * service to get a reference to this history object. Returns a pointer to
   * the service if it exists. Otherwise creates one. Returns NULL on error.
   */
  static nsNavHistory* GetHistoryService()
  {
    if (gHistoryService)
      return gHistoryService;

    nsCOMPtr<nsINavHistoryService> serv =
      do_GetService("@mozilla.org/browser/nav-history-service;1");
    NS_ENSURE_TRUE(serv, nsnull);

    return gHistoryService;
  }

#ifdef LAZY_ADD
  /**
   * Adds a lazy message for adding a favicon. Used by the favicon service so
   * that favicons are handled lazily just like page adds.
   */
  nsresult AddLazyLoadFaviconMessage(nsIURI* aPage, nsIURI* aFavicon,
                                     PRBool aForceReload);
#endif

  /**
   * Returns the database ID for the given URI, or 0 if not found and autoCreate
   * is false.
   */
  nsresult GetUrlIdFor(nsIURI* aURI, PRInt64* aEntryID,
                       PRBool aAutoCreate);

  nsresult CalculateFullVisitCount(PRInt64 aPlaceId, PRInt32 *aVisitCount);

  nsresult UpdateFrecency(PRInt64 aPlaceId, PRBool aIsBookmark);
  nsresult UpdateFrecencyInternal(PRInt64 aPlaceId, PRInt32 aTyped,
                                  PRInt32 aHidden, PRInt32 aOldFrecency,
                                  PRBool aIsBookmark);

  /**
   * Calculate frecencies for places that don't have a valid value yet
   */
  nsresult FixInvalidFrecencies();

  /**
   * Set the frecencies of excluded places so they don't show up in queries
   */
  nsresult FixInvalidFrecenciesForExcludedPlaces();

  /**
   * Returns a pointer to the storage connection used by history. This
   * connection object is also used by the annotation service and bookmarks, so
   * that things can be grouped into transactions across these components.
   *
   * NOT ADDREFed.
   *
   * This connection can only be used in the thread that created it the
   * history service!
   */
  mozIStorageConnection* GetStorageConnection()
  {
    return mDBConn;
  }

  /**
   * These functions return non-owning references to the locale-specific
   * objects for places components.
   */
  nsIStringBundle* GetBundle();
  nsIStringBundle* GetDateFormatBundle();
  nsICollation* GetCollation();
  void GetStringFromName(const PRUnichar* aName, nsACString& aResult);
  void GetAgeInDaysString(PRInt32 aInt, const PRUnichar *aName,
                          nsACString& aResult);
  void GetMonthName(PRInt32 aIndex, nsACString& aResult);

  // returns true if history has been disabled
  PRBool IsHistoryDisabled() { return mExpireDaysMax == 0 || InPrivateBrowsingMode(); }

  // get the statement for selecting a history row by URL
  mozIStorageStatement* DBGetURLPageInfo() { return mDBGetURLPageInfo; }

  // Constants for the columns returned by the above statement.
  static const PRInt32 kGetInfoIndex_PageID;
  static const PRInt32 kGetInfoIndex_URL;
  static const PRInt32 kGetInfoIndex_Title;
  static const PRInt32 kGetInfoIndex_RevHost;
  static const PRInt32 kGetInfoIndex_VisitCount;
  static const PRInt32 kGetInfoIndex_ItemId;
  static const PRInt32 kGetInfoIndex_ItemDateAdded;
  static const PRInt32 kGetInfoIndex_ItemLastModified;

  // select a history row by id
  mozIStorageStatement* DBGetIdPageInfo() { return mDBGetIdPageInfo; }

  mozIStorageStatement* DBGetTags() { return mDBGetTags; }
  PRInt64 GetTagsFolder();

  // Constants for the columns returned by the above statement
  // (in addition to the ones above).
  static const PRInt32 kGetInfoIndex_VisitDate;
  static const PRInt32 kGetInfoIndex_FaviconURL;

  // used in execute queries to get session ID info (only for visits)
  static const PRInt32 kGetInfoIndex_SessionId;

  // this actually executes a query and gives you results, it is used by
  // nsNavHistoryQueryResultNode
  nsresult GetQueryResults(nsNavHistoryQueryResultNode *aResultNode,
                           const nsCOMArray<nsNavHistoryQuery>& aQueries,
                           nsNavHistoryQueryOptions *aOptions,
                           nsCOMArray<nsNavHistoryResultNode>* aResults);

  // Take a row of kGetInfoIndex_* columns and construct a ResultNode.
  // The row must contain the full set of columns.
  nsresult RowToResult(mozIStorageValueArray* aRow,
                       nsNavHistoryQueryOptions* aOptions,
                       nsNavHistoryResultNode** aResult);
  nsresult QueryRowToResult(PRInt64 aItemId, const nsACString& aURI,
                            const nsACString& aTitle,
                            PRUint32 aAccessCount, PRTime aTime,
                            const nsACString& aFavicon,
                            nsNavHistoryResultNode** aNode);

  nsresult VisitIdToResultNode(PRInt64 visitId,
                               nsNavHistoryQueryOptions* aOptions,
                               nsNavHistoryResultNode** aResult);

  nsresult BookmarkIdToResultNode(PRInt64 aBookmarkId,
                                  nsNavHistoryQueryOptions* aOptions,
                                  nsNavHistoryResultNode** aResult);

  // used by other places components to send history notifications (for example,
  // when the favicon has changed)
  void SendPageChangedNotification(nsIURI* aURI, PRUint32 aWhat,
                                   const nsAString& aValue)
  {
    ENUMERATE_WEAKARRAY(mObservers, nsINavHistoryObserver,
                        OnPageChanged(aURI, aWhat, aValue));
  }

  // current time optimization
  PRTime GetNow();

  // well-known annotations used by the history and bookmarks systems
  static const char kAnnotationPreviousEncoding[];

  // used by query result nodes to update: see comment on body of CanLiveUpdateQuery
  static PRUint32 GetUpdateRequirements(const nsCOMArray<nsNavHistoryQuery>& aQueries,
                                        nsNavHistoryQueryOptions* aOptions,
                                        PRBool* aHasSearchTerms);
  PRBool EvaluateQueryForNode(const nsCOMArray<nsNavHistoryQuery>& aQueries,
                              nsNavHistoryQueryOptions* aOptions,
                              nsNavHistoryResultNode* aNode);

  static nsresult AsciiHostNameFromHostString(const nsACString& aHostName,
                                              nsACString& aAscii);
  void DomainNameFromURI(nsIURI* aURI,
                         nsACString& aDomainName);
  static PRTime NormalizeTime(PRUint32 aRelative, PRTime aOffset);

  // Don't use these directly, inside nsNavHistory use UpdateBatchScoper,
  // else use nsINavHistoryService::RunInBatchMode
  nsresult BeginUpdateBatch();
  nsresult EndUpdateBatch();

  // the level of nesting of batches, 0 when no batches are open
  PRInt32 mBatchLevel;

  // true if the outermost batch has an associated transaction that should
  // be committed when our batch level reaches 0 again.
  PRBool mBatchHasTransaction;

  // better alternative to QueryStringToQueries (in nsNavHistoryQuery.cpp)
  nsresult QueryStringToQueryArray(const nsACString& aQueryString,
                                   nsCOMArray<nsNavHistoryQuery>* aQueries,
                                   nsNavHistoryQueryOptions** aOptions);

  // Import-friendly version of AddVisit.
  // This method adds a page to history along with a single last visit.
  // aLastVisitDate can be -1 if there is no last visit date to record.
  //
  // This is only for use by the import of history.dat on first-run of Places,
  // which currently occurs if no places.sqlite file previously exists.
  nsresult AddPageWithVisits(nsIURI *aURI,
                             const nsString &aTitle,
                             PRInt32 aVisitCount,
                             PRInt32 aTransitionType,
                             PRTime aFirstVisitDate,
                             PRTime aLastVisitDate);

  // Checks the database for any duplicate URLs.  If any are found,
  // all but the first are removed.  This must be called after using
  // AddPageWithVisits, to ensure that the database is in a consistent state.
  nsresult RemoveDuplicateURIs();

  // sets the schema version in the database to match SCHEMA_VERSION
  nsresult UpdateSchemaVersion();

  // Returns true if we are currently in private browsing mode
  PRBool InPrivateBrowsingMode()
  {
    if (mInPrivateBrowsing == PRIVATEBROWSING_NOTINITED) {
      mInPrivateBrowsing = PR_FALSE;
      nsCOMPtr<nsIPrivateBrowsingService> pbs =
        do_GetService(NS_PRIVATE_BROWSING_SERVICE_CONTRACTID);
      if (pbs) {
        pbs->GetPrivateBrowsingEnabled(&mInPrivateBrowsing);
      }
    }

    return mInPrivateBrowsing;
  }

  typedef nsDataHashtable<nsCStringHashKey, nsCString> StringHash;

  /**
   * Helper method to finalize a statement
   */
  static nsresult
  FinalizeStatement(mozIStorageStatement *aStatement) {
    nsresult rv;
    if (aStatement) {
      rv = aStatement->Finalize();
      NS_ENSURE_SUCCESS(rv, rv);
    }
    return NS_OK;
  }

 private:
  ~nsNavHistory();

  // used by GetHistoryService
  static nsNavHistory* gHistoryService;

protected:

  //
  // Constants
  //
  nsCOMPtr<nsIPrefBranch> mPrefBranch; // MAY BE NULL when we are shutting down
  nsDataHashtable<nsStringHashKey, int> gExpandedItems;

  //
  // Database stuff
  //
  nsCOMPtr<mozIStorageService> mDBService;
  nsCOMPtr<mozIStorageConnection> mDBConn;
  nsCOMPtr<nsIFile> mDBFile;

  nsCOMPtr<mozIStorageStatement> mDBGetURLPageInfo;   // kGetInfoIndex_* results
  nsCOMPtr<mozIStorageStatement> mDBGetIdPageInfo;     // kGetInfoIndex_* results

  nsCOMPtr<mozIStorageStatement> mDBRecentVisitOfURL; // converts URL into most recent visit ID/session ID
  nsCOMPtr<mozIStorageStatement> mDBRecentVisitOfPlace; // converts placeID into most recent visit ID/session ID
  nsCOMPtr<mozIStorageStatement> mDBInsertVisit; // used by AddVisit
  nsCOMPtr<mozIStorageStatement> mDBGetPageVisitStats; // used by AddVisit
  nsCOMPtr<mozIStorageStatement> mDBIsPageVisited; // used by IsURIStringVisited
  nsCOMPtr<mozIStorageStatement> mDBUpdatePageVisitStats; // used by AddVisit
  nsCOMPtr<mozIStorageStatement> mDBAddNewPage; // used by InternalAddNewPage
  nsCOMPtr<mozIStorageStatement> mDBGetTags; // used by FilterResultSet
  nsCOMPtr<mozIStorageStatement> mFoldersWithAnnotationQuery;  // used by StartSearch and FilterResultSet
  nsCOMPtr<mozIStorageStatement> mDBSetPlaceTitle; // used by SetPageTitleInternal

  // these are used by VisitIdToResultNode for making new result nodes from IDs
  // Consumers need to use the getters since these statements are lazily created
  mozIStorageStatement *GetDBVisitToURLResult();
  nsCOMPtr<mozIStorageStatement> mDBVisitToURLResult; // kGetInfoIndex_* results
  mozIStorageStatement *GetDBVisitToVisitResult();
  nsCOMPtr<mozIStorageStatement> mDBVisitToVisitResult; // kGetInfoIndex_* results
  mozIStorageStatement *GetDBBookmarkToUrlResult();
  nsCOMPtr<mozIStorageStatement> mDBBookmarkToUrlResult; // kGetInfoIndex_* results

  /**
   * Finalize all internal statements.
   */
  nsresult FinalizeStatements();

  // nsICharsetResolver
  NS_DECL_NSICHARSETRESOLVER

  nsresult CalculateFrecency(PRInt64 aPageID, PRInt32 aTyped, PRInt32 aVisitCount, nsCAutoString &aURL, PRInt32 *aFrecency);
  nsresult CalculateFrecencyInternal(PRInt64 aPageID, PRInt32 aTyped, PRInt32 aVisitCount, PRBool aIsBookmarked, PRInt32 *aFrecency);
  nsCOMPtr<mozIStorageStatement> mDBVisitsForFrecency;
  nsCOMPtr<mozIStorageStatement> mDBUpdateFrecencyAndHidden;
  nsCOMPtr<mozIStorageStatement> mDBGetPlaceVisitStats;
  nsCOMPtr<mozIStorageStatement> mDBFullVisitCount;

  /**
   * Initializes the database file.  If the database does not exist, was
   * corrupted, or aForceInit is true, we recreate the database.  We also backup
   * the database if it was corrupted or aForceInit is true.
   *
   * @param aForceInit
   *        Indicates if we should close an open database connection or not.
   *        Note: A valid database connection must be opened if this is true.
   */
  nsresult InitDBFile(PRBool aForceInit);

  /**
   * Initializes the database.  This performs any necessary migrations for the
   * database.  All migration is done inside a transaction that is rolled back
   * if any error occurs.  Upon initialization, history is imported, and some
   * preferences that are used are set.
   */
  nsresult InitDB();
  nsresult InitTempTables();
  nsresult InitViews();
  nsresult InitFunctions();
  nsresult InitStatements();
  nsresult ForceMigrateBookmarksDB(mozIStorageConnection *aDBConn);
  nsresult MigrateV3Up(mozIStorageConnection *aDBConn);
  nsresult MigrateV6Up(mozIStorageConnection *aDBConn);
  nsresult MigrateV7Up(mozIStorageConnection *aDBConn);
  nsresult MigrateV8Up(mozIStorageConnection *aDBConn);
  nsresult MigrateV9Up(mozIStorageConnection *aDBConn);

  nsresult RemovePagesInternal(const nsCString& aPlaceIdsQueryString);

  nsresult AddURIInternal(nsIURI* aURI, PRTime aTime, PRBool aRedirect,
                          PRBool aToplevel, nsIURI* aReferrer);

  nsresult AddVisitChain(nsIURI* aURI, PRTime aTime,
                         PRBool aToplevel, PRBool aRedirect,
                         nsIURI* aReferrer, PRInt64* aVisitID,
                         PRInt64* aSessionID, PRInt64* aRedirectBookmark);
  nsresult InternalAddNewPage(nsIURI* aURI, const nsAString& aTitle,
                              PRBool aHidden, PRBool aTyped,
                              PRInt32 aVisitCount, PRBool aCalculateFrecency,
                              PRInt64* aPageID);
  nsresult InternalAddVisit(PRInt64 aPageID, PRInt64 aReferringVisit,
                            PRInt64 aSessionID, PRTime aTime,
                            PRInt32 aTransitionType, PRInt64* aVisitID);
  PRBool FindLastVisit(nsIURI* aURI, PRInt64* aVisitID,
                       PRInt64* aSessionID);
  PRBool IsURIStringVisited(const nsACString& url);

  /**
   * This loads all of the preferences that we use into member variables.
   * NOTE:  If mPrefBranch is NULL, this does nothing.
   *
   * @param aInitializing
   *        Indicates if the autocomplete queries should be regenerated or not.
   */
  nsresult LoadPrefs(PRBool aInitializing);

  // Current time optimization
  PRTime mLastNow;
  PRBool mNowValid;
  nsCOMPtr<nsITimer> mExpireNowTimer;
  static void expireNowTimerCallback(nsITimer* aTimer, void* aClosure);

  // expiration
  friend class nsNavHistoryExpire;
  nsNavHistoryExpire mExpire;

#ifdef LAZY_ADD
  // lazy add committing
  struct LazyMessage {
    enum MessageType { Type_Invalid, Type_AddURI, Type_Title, Type_Favicon };
    LazyMessage()
    {
      type = Type_Invalid;
      isRedirect = PR_FALSE;
      isToplevel = PR_FALSE;
      time = 0;
      alwaysLoadFavicon = PR_FALSE;
    }

    // call this with common parms to initialize. Caller is responsible for
    // setting other elements manually depending on type.
    nsresult Init(MessageType aType, nsIURI* aURI)
    {
      NS_ENSURE_ARG_POINTER(aURI);
      type = aType;
      nsresult rv = aURI->Clone(getter_AddRefs(uri));
      NS_ENSURE_SUCCESS(rv, rv);
      return uri->GetSpec(uriSpec);
    }

    // common elements
    MessageType type;
    nsCOMPtr<nsIURI> uri;
    nsCString uriSpec; // stringified version of URI, for quick isVisited

    // valid when type == Type_AddURI
    nsCOMPtr<nsIURI> referrer;
    PRBool isRedirect;
    PRBool isToplevel;
    PRTime time;

    // valid when type == Type_Title
    nsString title;

    // valid when type == LAZY_FAVICON
    nsCOMPtr<nsIURI> favicon;
    PRBool alwaysLoadFavicon;
  };
  nsTArray<LazyMessage> mLazyMessages;
  nsCOMPtr<nsITimer> mLazyTimer;
  PRBool mLazyTimerSet;
  PRUint32 mLazyTimerDeferments; // see StartLazyTimer
  nsresult StartLazyTimer();
  nsresult AddLazyMessage(const LazyMessage& aMessage);
  static void LazyTimerCallback(nsITimer* aTimer, void* aClosure);
  void CommitLazyMessages();
#endif

  nsresult ConstructQueryString(const nsCOMArray<nsNavHistoryQuery>& aQueries, 
                                nsNavHistoryQueryOptions* aOptions,
                                nsCString& queryString,
                                PRBool& aParamsPresent,
                                StringHash& aAddParams);

  nsresult QueryToSelectClause(nsNavHistoryQuery* aQuery,
                               nsNavHistoryQueryOptions* aOptions,
                               PRInt32 aQueryIndex,
                               nsCString* aClause);
  nsresult BindQueryClauseParameters(mozIStorageStatement* statement,
                                     PRInt32 aQueryIndex,
                                     nsNavHistoryQuery* aQuery,
                                     nsNavHistoryQueryOptions* aOptions);

  nsresult ResultsAsList(mozIStorageStatement* statement,
                         nsNavHistoryQueryOptions* aOptions,
                         nsCOMArray<nsNavHistoryResultNode>* aResults);

  void TitleForDomain(const nsCString& domain, nsACString& aTitle);

  nsresult SetPageTitleInternal(nsIURI* aURI, const nsAString& aTitle);

  nsresult FilterResultSet(nsNavHistoryQueryResultNode *aParentNode,
                           const nsCOMArray<nsNavHistoryResultNode>& aSet,
                           nsCOMArray<nsNavHistoryResultNode>* aFiltered,
                           const nsCOMArray<nsNavHistoryQuery>& aQueries,
                           nsNavHistoryQueryOptions* aOptions);

  // observers
  nsMaybeWeakPtrArray<nsINavHistoryObserver> mObservers;

  // effective tld service
  nsCOMPtr<nsIEffectiveTLDService> mTLDService;
  nsCOMPtr<nsIIDNService>          mIDNService;

  // localization
  nsCOMPtr<nsIStringBundle> mBundle;
  nsCOMPtr<nsIStringBundle> mDateFormatBundle;
  nsCOMPtr<nsICollation> mCollation;

  // annotation service : MAY BE NULL!
  //nsCOMPtr<mozIAnnotationService> mAnnotationService;

  // recent events
  typedef nsDataHashtable<nsCStringHashKey, PRInt64> RecentEventHash;
  RecentEventHash mRecentTyped;
  RecentEventHash mRecentBookmark;

  PRBool CheckIsRecentEvent(RecentEventHash* hashTable,
                            const nsACString& url);
  void ExpireNonrecentEvents(RecentEventHash* hashTable);

  // redirect tracking. See GetRedirectFor for a description of how this works.
  struct RedirectInfo {
    nsCString mSourceURI;
    PRTime mTimeCreated;
    PRUint32 mType; // one of TRANSITION_REDIRECT_[TEMPORARY,PERMANENT]
  };
  typedef nsDataHashtable<nsCStringHashKey, RedirectInfo> RedirectHash;
  RedirectHash mRecentRedirects;
  static PLDHashOperator ExpireNonrecentRedirects(
      nsCStringHashKey::KeyType aKey, RedirectInfo& aData, void* aUserArg);
  PRBool GetRedirectFor(const nsACString& aDestination, nsACString& aSource,
                        PRTime* aTime, PRUint32* aRedirectType);

  // session tracking
  PRInt64 mLastSessionID;
  PRInt64 GetNewSessionID() { mLastSessionID ++; return mLastSessionID; }

  //
  // AutoComplete stuff
  //
  static const PRInt32 kAutoCompleteIndex_URL;
  static const PRInt32 kAutoCompleteIndex_Title;
  static const PRInt32 kAutoCompleteIndex_FaviconURL;
  static const PRInt32 kAutoCompleteIndex_ParentId;
  static const PRInt32 kAutoCompleteIndex_BookmarkTitle;
  static const PRInt32 kAutoCompleteIndex_Tags;
  static const PRInt32 kAutoCompleteIndex_VisitCount;
  static const PRInt32 kAutoCompleteIndex_Typed;
  nsCOMPtr<mozIStorageStatement> mDBCurrentQuery; //  kAutoCompleteIndex_* results
  nsCOMPtr<mozIStorageStatement> mDBAutoCompleteQuery; //  kAutoCompleteIndex_* results
  nsCOMPtr<mozIStorageStatement> mDBAutoCompleteTypedQuery; //  kAutoCompleteIndex_* results
  mozIStorageStatement* GetDBAutoCompleteHistoryQuery();
  nsCOMPtr<mozIStorageStatement> mDBAutoCompleteHistoryQuery; //  kAutoCompleteIndex_* results
  mozIStorageStatement* GetDBAutoCompleteStarQuery();
  nsCOMPtr<mozIStorageStatement> mDBAutoCompleteStarQuery; //  kAutoCompleteIndex_* results
  mozIStorageStatement* GetDBAutoCompleteTagsQuery();
  nsCOMPtr<mozIStorageStatement> mDBAutoCompleteTagsQuery; //  kAutoCompleteIndex_* results
  nsCOMPtr<mozIStorageStatement> mDBPreviousQuery; //  kAutoCompleteIndex_* results
  nsCOMPtr<mozIStorageStatement> mDBAdaptiveQuery; //  kAutoCompleteIndex_* results
  nsCOMPtr<mozIStorageStatement> mDBKeywordQuery; //  kAutoCompleteIndex_* results
  mozIStorageStatement* GetDBFeedbackIncrease();
  nsCOMPtr<mozIStorageStatement> mDBFeedbackIncrease;

  /**
   * AutoComplete word matching behavior to determine if words should match on
   * word boundaries or not or both.
   */
  enum MatchType {
    MATCH_ANYWHERE,
    MATCH_BOUNDARY_ANYWHERE,
    MATCH_BOUNDARY,
    MATCH_BEGINNING
  };

  nsresult InitAutoComplete();
  nsresult CreateAutoCompleteQueries();
  PRBool mAutoCompleteEnabled;
  MatchType mAutoCompleteMatchBehavior;
  PRBool mAutoCompleteFilterJavascript;
  PRInt32 mAutoCompleteMaxResults;
  nsString mAutoCompleteRestrictHistory;
  nsString mAutoCompleteRestrictBookmark;
  nsString mAutoCompleteRestrictTag;
  nsString mAutoCompleteMatchTitle;
  nsString mAutoCompleteMatchUrl;
  nsString mAutoCompleteRestrictTyped;
  PRInt32 mAutoCompleteSearchChunkSize;
  PRInt32 mAutoCompleteSearchTimeout;
  nsCOMPtr<nsITimer> mAutoCompleteTimer;

  static const PRInt32 kAutoCompleteBehaviorHistory;
  static const PRInt32 kAutoCompleteBehaviorBookmark;
  static const PRInt32 kAutoCompleteBehaviorTag;
  static const PRInt32 kAutoCompleteBehaviorTitle;
  static const PRInt32 kAutoCompleteBehaviorUrl;
  static const PRInt32 kAutoCompleteBehaviorTyped;

  PRInt32 mAutoCompleteDefaultBehavior; // kAutoCompleteBehavior* bitmap
  PRInt32 mAutoCompleteCurrentBehavior; // kAutoCompleteBehavior* bitmap

  // Original search string for case-sensitive usage
  nsString mOrigSearchString;
  // Search string and tokens for case-insensitive matching
  nsString mCurrentSearchString;
  nsTArray<nsString> mCurrentSearchTokens;
  void GenerateSearchTokens();
  void AddSearchToken(nsAutoString &aToken);
  void ProcessTokensForSpecialSearch();

#ifdef MOZ_XUL
  nsresult AutoCompleteFeedback(PRInt32 aIndex,
                                nsIAutoCompleteController *aController);

  nsCOMPtr<nsIAutoCompleteObserver> mCurrentListener;
  nsCOMPtr<nsIAutoCompleteSimpleResult> mCurrentResult;
#endif

  MatchType mCurrentMatchType;
  MatchType mPreviousMatchType;
  nsDataHashtable<nsStringHashKey, PRBool> mCurrentResultURLs;
  PRInt32 mCurrentChunkOffset;
  PRInt32 mPreviousChunkOffset;

  nsDataHashtable<nsTrimInt64HashKey, PRBool> mLivemarkFeedItemIds;
  nsDataHashtable<nsStringHashKey, PRBool> mLivemarkFeedURIs;

  nsresult AutoCompleteFullHistorySearch(PRBool* aHasMoreResults);
  nsresult AutoCompletePreviousSearch();
  nsresult AutoCompleteAdaptiveSearch();
  nsresult AutoCompleteKeywordSearch();

  /**
   * Query type passed to AutoCompleteProcessSearch to determine what style to
   * use and if results should be filtered
   */
  enum QueryType {
    QUERY_KEYWORD,
    QUERY_FILTERED
  };
  nsresult AutoCompleteProcessSearch(mozIStorageStatement* aQuery,
                                     const QueryType aType,
                                     PRBool *aHasMoreResults = nsnull);
  PRBool AutoCompleteHasEnoughResults();

  nsresult PerformAutoComplete();
  nsresult StartAutoCompleteTimer(PRUint32 aMilliseconds);
  static void AutoCompleteTimerCallback(nsITimer* aTimer, void* aClosure);

  PRBool mAutoCompleteFinishedSearch;
  void DoneSearching(PRBool aFinished);

  // Used to unescape encoded URI strings for searching
  nsCOMPtr<nsITextToSubURI> mTextURIService;
  nsString FixupURIText(const nsAString &aURIText);

  PRInt32 mExpireDaysMin;
  PRInt32 mExpireDaysMax;
  PRInt32 mExpireSites;

  // frecency prefs
  PRInt32 mNumVisitsForFrecency;
  PRInt32 mFirstBucketCutoffInDays;
  PRInt32 mSecondBucketCutoffInDays;
  PRInt32 mThirdBucketCutoffInDays;
  PRInt32 mFourthBucketCutoffInDays;
  PRInt32 mFirstBucketWeight;
  PRInt32 mSecondBucketWeight;
  PRInt32 mThirdBucketWeight;
  PRInt32 mFourthBucketWeight;
  PRInt32 mDefaultWeight;
  PRInt32 mEmbedVisitBonus;
  PRInt32 mLinkVisitBonus;
  PRInt32 mTypedVisitBonus;
  PRInt32 mBookmarkVisitBonus;
  PRInt32 mDownloadVisitBonus;
  PRInt32 mPermRedirectVisitBonus;
  PRInt32 mTempRedirectVisitBonus;
  PRInt32 mDefaultVisitBonus;
  PRInt32 mUnvisitedBookmarkBonus;
  PRInt32 mUnvisitedTypedBonus;

  // in nsNavHistoryQuery.cpp
  nsresult TokensToQueries(const nsTArray<QueryKeyValuePair>& aTokens,
                           nsCOMArray<nsNavHistoryQuery>* aQueries,
                           nsNavHistoryQueryOptions* aOptions);

  /**
   * Used to setup the idle timer used to perform various tasks when the user is
   * idle..
   */
  nsCOMPtr<nsITimer> mIdleTimer;
  nsresult InitializeIdleTimer();
  static void IdleTimerCallback(nsITimer* aTimer, void* aClosure);
  nsresult OnIdle();

  PRInt64 mTagsFolder;

  PRBool mInPrivateBrowsing;

  PRUint16 mDatabaseStatus;
};

/**
 * Shared between the places components, this function binds the given URI as
 * UTF8 to the given parameter for the statement.
 */
nsresult BindStatementURI(mozIStorageStatement* statement, PRInt32 index,
                          nsIURI* aURI);

#define PLACES_URI_PREFIX "place:"

/* Returns true if the given URI represents a history query. */
inline PRBool IsQueryURI(const nsCString &uri)
{
  return StringBeginsWith(uri, NS_LITERAL_CSTRING(PLACES_URI_PREFIX));
}

/* Extracts the query string from a query URI. */
inline const nsDependentCSubstring QueryURIToQuery(const nsCString &uri)
{
  NS_ASSERTION(IsQueryURI(uri), "should only be called for query URIs");
  return Substring(uri, NS_LITERAL_CSTRING(PLACES_URI_PREFIX).Length());
}

#endif // nsNavHistory_h_
